//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "Node.h"
#include <string>
#include <fstream>
#include <sstream>

#include "MyCustomMsg_m.h"

Define_Module(Node);

void Node::initialize()
{
    // TODO - Generated method body

    // print network parameters
    WS = getParentModule()->par("WS");
    TO = getParentModule()->par("TO");
    PT = getParentModule()->par("PT");
    TD = getParentModule()->par("TD");
    ED = getParentModule()->par("ED");
    DD = getParentModule()->par("DD");
    LP = getParentModule()->par("LP");
//    myBuffer = new std::pair<std::string, std::string>[WS+1];
    myBuffer.resize(WS+1);
    Timers.resize(WS+1);
    endWindowIndex = WS-1;

    EV << "WS = " << WS << endl;
    EV << "TO = " << TO << endl;
    EV << "PT = " << PT << endl;
    EV << "LP = " << LP << endl;
    EV << "TD = " << TD << endl;
    EV << "ED = " << ED << endl;
    EV << "DD = " << DD << endl;
}

void Node::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    MyCustomMsg_Base *mmsg = check_and_cast<MyCustomMsg_Base *>(msg);
    bool isCoordinator =checkCoordinator(mmsg);
    std::string identifier;
    std::string payload;
    if (isCoordinator)
    {
        // start sending if sender else do nothing
        if(isSending)
        {
            file = openFile("../input" + std::to_string(getIndex()) + ".txt");
            msg->setKind(0);
            scheduleAt(simTime()+startTime,msg);
        }

    }
    // check i am sender or receive
    else if(isSending)
    {
        // sending
        if(msg->isSelfMessage())
        {
            EV << msg->getKind() << checkSeqBetween(startWindowIndex, endWindowIndex, currentWindowIndex);
            if(msg->getKind() == 0 && checkSeqBetween(startWindowIndex, endWindowIndex, currentWindowIndex))
            {
                EV << "Node "<< getIndex() << " is sender"<<endl;
                std::pair<std::string, std::string> line = readNextLine(file);
                identifier = line.first;
                payload = line.second;
                if (identifier.empty() && payload.empty())
                {
                    EV << "File reading is done is done";
//                    endSimulation();
                }
                else
                {
                    mmsg->setHeader(currentWindowIndex);
                    EV << identifier << endl;
                    EV << payload << endl;
                    myBuffer[currentWindowIndex] = line;
                    std::string frame = Framing(payload);
                    EV << frame;
                    std::bitset<8> parity_byte = Checksum(frame);
                    char trailer = static_cast<char>(parity_byte.to_ulong());
                    mmsg->setTrailer(trailer);
                    // Switch cases on identifier
                    checkCases(identifier,mmsg,frame);
                    if (Timers[currentWindowIndex] != NULL){
                        if(Timers[currentWindowIndex]->isScheduled()) // check if timer is scheduled
                        {
                           cancelAndDelete(Timers[currentWindowIndex]); // delete the timer message
                           Timers[currentWindowIndex] = NULL;
                        }
                    }
//                    MyCustomMsg_Base* timeOutMsg = new MyCustomMsg_Base("time out");
                    Timers[currentWindowIndex] = new MyCustomMsg_Base("time out");
                    Timers[currentWindowIndex]->setKind(1);
                    scheduleAt(simTime()+PT+TO,Timers[currentWindowIndex]);
                    incrementSequenceNo();
                 }
            }
            else if(msg->getKind() == 1)
            {
                // time out
                timeOutHandling();
                //cancelAndDelete(mmsg);

            }
            else if(msg->getKind() == 2) // NACK
            {
                 payload = myBuffer[mmsg->getHeader()].second;
                 identifier = myBuffer[mmsg->getHeader()].first;
                 mmsg->setHeader(mmsg->getHeader());
                 EV << identifier << 2<< endl;
                 EV << payload << endl;
                 std::string frame = Framing(payload);
                 EV << frame;
                 std::bitset<8> parity_byte = Checksum(frame);
                 char trailer = static_cast<char>(parity_byte.to_ulong());
                 mmsg->setTrailer(trailer);
                 // Switch cases on identifier
                 checkCases("0000",mmsg,frame);
                 if (Timers[mmsg->getHeader()] != nullptr){
                     if(Timers[mmsg->getHeader()]->isScheduled()) // check if timer is scheduled
                     {
                        cancelAndDelete(Timers[mmsg->getHeader()]); // delete the timer message
                        Timers[mmsg->getHeader()] = NULL;
                     }
                 }
                 Timers[mmsg->getHeader()] = new MyCustomMsg_Base("time out");
                 Timers[mmsg->getHeader()]->setKind(1);
                 scheduleAt(simTime()+PT+TO,Timers[mmsg->getHeader()]);
            }
            else if(msg->getKind() == 3) // normal case
            {
                payload = myBuffer[mmsg->getHeader()].second;
                identifier = myBuffer[mmsg->getHeader()].first;
                mmsg->setHeader(mmsg->getHeader());
                EV << identifier << 3<< endl;
                EV << payload << endl;
                std::string frame = Framing(payload);
                EV << frame;
                std::bitset<8> parity_byte = Checksum(frame);
                char trailer = static_cast<char>(parity_byte.to_ulong());
                mmsg->setTrailer(trailer);
                // Switch cases on identifier
                checkCases(identifier,mmsg,frame);
                if (Timers[mmsg->getHeader()] != NULL){
                    if(Timers[mmsg->getHeader()]->isScheduled()) // check if timer is scheduled
                    {
                       cancelAndDelete(Timers[mmsg->getHeader()]); // delete the timer message
                       Timers[mmsg->getHeader()] = NULL;
                    }
                }
                Timers[mmsg->getHeader()] = new MyCustomMsg_Base("time out");
                Timers[mmsg->getHeader()]->setKind(1);
                scheduleAt(simTime()+PT+TO,Timers[mmsg->getHeader()]);
            }
            return;
        }
        else{
            // Ack/Nack
            if(mmsg->getFrame_Type() == 1)
            {
                handleACK(mmsg);
            }
            else if(mmsg->getFrame_Type() == 0)
            {
                handleNACK(mmsg);
            }
        }

    }
    else
    {
        // receiving from sender message
        receivePacket(mmsg);
    }
}
void Node::handleACK(MyCustomMsg_Base* msg)
{
    EV <<"handleACK"<<endl;
    startWindowIndex = incrementWindowNo(startWindowIndex);
    endWindowIndex = incrementWindowNo(endWindowIndex);
    int frame_number = (msg->getAck_Nack_Num() - 1)%(WS + 1);
    std::cout<<"Frame  num = "<<frame_number<<" msg->getHeader() = "<<msg->getAck_Nack_Num()<<endl;
    if (Timers[frame_number] != NULL){
        if(Timers[frame_number]->isScheduled()) // check if timer is scheduled
        {
           EV<<"Stopped timer for frame "<<frame_number<<endl; // if scheduled, cancel it
           cancelAndDelete(Timers[frame_number]); // delete the timer message
           Timers[frame_number] = NULL;
        }
    }
    else
    {
        std::cout<<"I am here"<<endl;
    }
    MyCustomMsg_Base* selfMessage2 = new MyCustomMsg_Base("self message");
    selfMessage2->setKind(0);
    scheduleAt(simTime(),selfMessage2);
}
void Node::handleNACK(MyCustomMsg_Base* msg)
{
       EV <<"handleNACK"<<endl;
//    currentWindowIndex = msg->getHeader();
    MyCustomMsg_Base* selfMessage = new MyCustomMsg_Base("self message");
    selfMessage->setKind(2);
    selfMessage->setHeader(startWindowIndex);
    scheduleAt(simTime()+0.001,selfMessage);
    if (Timers[startWindowIndex] != NULL){
        if(Timers[startWindowIndex]->isScheduled()) // check if timer is scheduled
         {
           cancelAndDelete(Timers[startWindowIndex]);
           Timers[startWindowIndex] = NULL;
         }
    }
    int index = incrementWindowNo(startWindowIndex);
    while(index!=currentWindowIndex)
    {
        MyCustomMsg_Base* selfMessage = new MyCustomMsg_Base("self message");
        selfMessage->setKind(3);
        selfMessage->setHeader(index);
        scheduleAt(simTime()+abs(index-startWindowIndex)*PT,selfMessage);
        if (Timers[index] != nullptr){
            if(Timers[index]->isScheduled()) // check if timer is scheduled
             {
                cancelAndDelete(Timers[index]);
                Timers[index] = NULL;
             }
        }
        index = incrementWindowNo(index);
    }
    MyCustomMsg_Base* selfMessage2 = new MyCustomMsg_Base("self message");
    selfMessage2->setKind(0);
    scheduleAt(simTime()+abs(index-startWindowIndex)*PT,selfMessage2);
}
std::ifstream Node::openFile(const std::string &fileName)
{
    std::ifstream file(fileName);
    if (!file.is_open())
    {
        std::cerr << "Unable to open file " << fileName << std::endl;
    }
    return file;
}
std::pair<std::string, std::string> Node::readNextLine(std::ifstream & file)
{
    std::string line;
    if (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string identifier, content;
        // Extract the identifier
        iss >> identifier;
        // Extract the rest of the line as content
        std::getline(iss >> std::ws, content);
        return std::make_pair(identifier, content);
    }
    else
    {
        return std::make_pair("", "");
    }

}


void Node::receivePacket(MyCustomMsg_Base* msg)
{
    // check if seq no is expected to have
    // if true check message if there is error
    //      if error then send Nack
    //      else send ack
    // else do not respond with message
    if (seqNumToReceive == msg->getHeader())
    {
        // receiving from sender message
        std::string frame = msg->getPayload(); // message.getpayload
        char trailer = msg->getTrailer(); // message.gettrailer
        std::cout << "Frame in reciever = "<<frame<<endl;
        bool errored_frame = ErrorDetection(frame, trailer);
        if (errored_frame)
        {
            // send Nack to sender with same header with probability LP
            msg->setAck_Nack_Num(seqNumToReceive);
            msg->setFrame_Type(0);
            send(msg,"out");
            EV << "Receiver: error in frame no"<< seqNumToReceive<<endl;
            std::string logs = "At time["+std::to_string(PT) +"], Node["+std::to_string(getIndex())+"] Sending [NACK] with number ["+
                                          std::to_string(seqNumToReceive)+"] , loss [Yes]";
            logStates(logs);
        }
        else
        {
            int previousSeqNum  = seqNumToReceive;
            std::cout <<"seqNumToReceive= " << seqNumToReceive << "WS = " << WS << endl;
            seqNumToReceive = incrementWindowNo(seqNumToReceive);
            std::cout <<"seqNumToReceive= " << seqNumToReceive << endl;
            std::string payload = Deframing(frame);
            msg->setFrame_Type(1);
            msg->setAck_Nack_Num(seqNumToReceive);
            send(msg,"out");
            EV << "Receiver: message received "<< payload<<endl;

            // print payload
            std::string logs = "At time["+std::to_string(PT) +"], Node["+std::to_string(getIndex())+"] Sending [ACK] with number ["+
                                                      std::to_string(seqNumToReceive)+"] , loss [Yes]";
            logStates(logs);
            // print payload
            logs = "Uploading payload=["+payload+"] and seq_num =["+std::to_string(previousSeqNum)+"] to the network layer";
            logStates(logs);
        }


    }
}
std::string Node::Modification(std::string message)
{
    int message_length = message.size();
    std::vector<std::bitset<8> >message_bit_stream;
    for(int i=0; i<message_length;i++)
    {
        std::bitset<8> bit_message(message[i]);
        message_bit_stream.push_back(bit_message);
    }
    int errorBit = int(uniform(0, message.size()*8));
    int errorChar = errorBit/8;
    message_bit_stream[errorChar][errorBit%8] = !message_bit_stream[errorChar][errorBit%8];
    std::string errored_message = "";
    for(std::vector<std::bitset<8> >::iterator it = message_bit_stream.begin(); it != message_bit_stream.end(); ++it)
    {
        errored_message +=(char)(*it).to_ulong();
    }

    return errored_message;
}
std::string Node::Framing(std::string message)
{
    std::string frame ="";
    for(int i=0;i<message.size();i++)
    {
        if(message[i] == '\\' || message[i] == '$')
        {
           frame += '\\';
        }
        frame += message[i];
    }
    frame = "$" + frame + "$";
    return frame;
}
std::bitset<8> Node::Checksum(std::string frame)
{
    int checksum = 0;
    int frame_int;
    for(int i=0;i<frame.size();i++)
    {
        std::bitset<8> frame_byte(frame[i]);
        frame_int = (frame_byte).to_ulong();
        checksum += frame_int;
        if(checksum > 255)
        {
            checksum = checksum - 256 + 1;
        }
    }
    std::bitset<8> checksum_bit_stream(checksum);
    return ~checksum_bit_stream;
}
std::string Node::Deframing(std::string frame)
{
    std::string payload;
    std::cout<<"Frame = "<<frame<<endl;
    for(int i=1;i<frame.size()-1;i++)
    {
        if(frame[i] == '\\')
            continue;
        payload += frame[i];
    }
    return payload;
}
bool Node::ErrorDetection(std::string frame, char parity_byte)
{
    return Checksum(frame) != std::bitset<8>(parity_byte);
}
void Node::timeOutHandling()
{
    EV <<"timeOutHandling"<<endl;
    // resend all messages from start window to current index
    // resend first message with free error then other in no error
    MyCustomMsg_Base* msgToSend = new MyCustomMsg_Base();

    msgToSend->setHeader(startWindowIndex);
    std::string frame = Framing(myBuffer[startWindowIndex].second);
    std::bitset<8> parity_byte = Checksum(frame);
    char trailer = static_cast<char>(parity_byte.to_ulong());
    msgToSend->setTrailer(trailer);
    msgToSend->setPayload(frame.c_str());
    checkCases("0000",msgToSend,frame);

    int index = incrementWindowNo(startWindowIndex);
    while(index!=currentWindowIndex)
    {
        MyCustomMsg_Base* msgToSend = new MyCustomMsg_Base();

        msgToSend->setHeader(startWindowIndex);
        std::string frame = Framing(myBuffer[startWindowIndex].second);
        std::bitset<8> parity_byte = Checksum(frame);
        char trailer = static_cast<char>(parity_byte.to_ulong());
        msgToSend->setTrailer(trailer);
        msgToSend->setPayload(frame.c_str());
        checkCases(myBuffer[index].first,msgToSend,frame);
        index = incrementWindowNo(index);
    }

}



void Node::checkCases(const std::string& identifier,MyCustomMsg_Base* msg,std::string frame)
{
    unsigned int inputValue = std::bitset<4>(identifier).to_ulong();
    std::string modified_frame;

    // Switch-case logic based on the input value
    MyCustomMsg_Base* selfMessage = new MyCustomMsg_Base("self message");
    selfMessage->setKind(0);
    scheduleAt(simTime()+PT,selfMessage);

    switch (inputValue) {
        case 0b0000:
            msg->setPayload(frame.c_str());
            sendDelayed(msg, PT+TD,"out");
            break;
        case 0b0001:
            msg->setPayload(frame.c_str());
            sendDelayed(msg, PT+TD+ED, "out");
            break;
        case 0b0010:
            msg->setPayload(frame.c_str());
            sendDelayed(msg, PT+TD, "out");
            sendDelayed(msg, PT+TD+DD, "out");
            break;
        case 0b0011:
            msg->setPayload(frame.c_str());
            sendDelayed(msg, PT+TD+ED, "out");
            sendDelayed(msg, PT+TD+DD+ED, "out");
            break;
        case 0b0100: // loss
            break;
        case 0b0101:
            break;
        case 0b0110:
            break;
        case 0b0111:
            break;
        case 0b1000:
            modified_frame = Modification(frame);
            msg->setPayload(modified_frame.c_str());
            sendDelayed(msg, PT+TD, "out");
            break;
        case 0b1001:
            modified_frame = Modification(frame);
            msg->setPayload(modified_frame.c_str());
            sendDelayed(msg, PT+TD+ED, "out");
            break;
        case 0b1010:
            modified_frame = Modification(frame);
            msg->setPayload(modified_frame.c_str());
            sendDelayed(msg, PT+TD, "out");
            sendDelayed(new MyCustomMsg_Base(*msg), PT+TD+DD, "out");
            break;
        case 0b1011: // Mod, dup,delay
            modified_frame = Modification(frame);
            msg->setPayload(modified_frame.c_str());
            sendDelayed(msg, PT+TD+ED, "out");
            sendDelayed(msg, PT+TD+DD+ED, "out");
            break;
        case 0b1100: // loss, mod
            break;
        case 0b1101: // loss, mod, delay
            break;
        case 0b1110: // loss, mod, dup
            break;
        case 0b1111: // loss, mod, dup, delay
            break;

        default: break;
    }

}


void Node::logStates(std::string logs)
{
    // Open the file in append mode
        std::ofstream outputFile("output.txt", std::ios::app);

        // Check if the file is successfully opened
        if (!outputFile.is_open()) {
            std::cerr << "Error opening the file!" << std::endl;
        }

        outputFile << logs << std::endl;

        // Close the file
        outputFile.close();

        std::cout << "String appended to the file successfully." << std::endl;
}
void Node::incrementSequenceNo()
{
    if (currentWindowIndex+1 > WS)
    {
        currentWindowIndex =0;
    }
    else
    {
        currentWindowIndex++;
    }
}
int Node::incrementWindowNo(int number)
{
    if (number+1 > WS)
    {
        number = 0;
    }
    else
    {
        number++;
    }
    return number;
}
bool Node::checkSeqBetween(int start,int end,int seq)
{
    EV <<endl<< "start" << start << "end" <<end<<"seq"<<seq<<endl;
    if( ((start<= seq )&& (seq < end)) || ((end < start )&& (start <= seq))
            || ((seq < end )&& (end < start))  || seq == end)
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool Node::checkCoordinator(MyCustomMsg_Base* msg)
{
    int senderID = msg->getSenderModuleId();
    // check if coordinator ssending
    if (senderID ==2)
    {
        // coordinator sending
        EV << "coordinator sending "<<endl;
        int nodeSending = msg->getAck_Nack_Num();
        if(nodeSending == getIndex())
        {
            // sender
            isSending = true;
            startTime = atoi(msg->getPayload());
            EV << "Node "<< getIndex() << " is sender"<<endl;
            EV << "sending at time: "<<startTime<<endl;

            // set parameter of sender;
        }
        else
        {
            isSending = false;
            EV << "Node "<< getIndex() << " is receiver"<<endl;
            // set parameter of receiver
            //WS = 1;

        }
        return true;
    }
    return false;
}
