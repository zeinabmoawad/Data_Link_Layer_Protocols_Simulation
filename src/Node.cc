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
    outputFileName = "../src/output.txt";


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
            std::ifstream file(outputFileName);
                        if (file.good()) { // Check if the file exists
                            file.close();

                            // Open the file in output mode to truncate it
                            std::ofstream ofs(outputFileName, std::ofstream::out | std::ofstream::trunc);
                            ofs.close();
//                            std::cout << "File '" << outputFileName << "' exists and has been emptied." << std::endl;
                        } else {
//                            std::cout << "File '" << outputFileName << "' does not exist." << std::endl;
                        }
            msg->setKind(0);
            scheduleAt(simTime()+startTime,msg);
            std::cout<<"At the beginning at time = "<< simTime().dbl()+startTime<<endl;
        }

    }
    // check i am sender or receive
    else if(isSending)
    {
        // sending
        if(msg->isSelfMessage())
        {
//            printBuffer();
//            EV << msg->getKind() << checkSeqBetween(startWindowIndex, endWindowIndex, currentWindowIndex);
            std::cout<<"I am sender self message  startWindowIndex = " <<startWindowIndex<<" currentWindowIndex = "<<currentWindowIndex<<" endWindowIndex= "<<endWindowIndex<<endl;
            if(msg->getKind() == 0 && checkSeqBetween2(startWindowIndex, endWindowIndex, currentWindowIndex))
            {
//                EV << "Node "<< getIndex() << " is sender"<<endl;
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
                    MyCustomMsg_Base* send_msg = new MyCustomMsg_Base("send message");
                    std::string logs = "At time["+std::to_string(simTime().dbl()) +"], Node["+std::to_string(getIndex())+"], Introducing channel error with code = [ "+identifier+" ]";
                                   logStates(logs);
                    send_msg->setHeader(currentWindowIndex);
//                    EV << identifier << endl;
//                    EV << payload << endl;
//                    std::cout<<"current window index reading new line "<<currentWindowIndex <<endl;
                    std::cout<<"I am reading new msg =  " <<payload<<" seq = "<<currentWindowIndex<<endl;
                    myBuffer[currentWindowIndex] = line;
                    std::string frame = Framing(payload);
//                    EV << frame;
                    std::bitset<8> parity_byte = Checksum(frame);
                    char trailer = static_cast<char>(parity_byte.to_ulong());
                    send_msg->setTrailer(trailer);
                    // Switch cases on identifier
                    checkCases(identifier,send_msg,frame);
                    MyCustomMsg_Base* selfMessage = new MyCustomMsg_Base("self message1");
                    selfMessage->setKind(0);
                    scheduleAt(simTime()+PT,selfMessage);
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
                std::cout<<"self msg for timeout = "<< simTime().dbl()<<endl;
                timeOutHandling();
                //cancelAndDelete(mmsg);

            }
            else if(msg->getKind() == 2) // NACK
            {
                 payload = myBuffer[mmsg->getHeader()].second;
                 identifier = myBuffer[mmsg->getHeader()].first;
                std::cout<<"Handling error free after NACK/TIMEOUT = "<< simTime().dbl()<<" payload = "<<payload<<endl;
                 mmsg->setHeader(mmsg->getHeader());
//                 EV << identifier << 2<< endl;
//                 EV << payload << endl;
                 std::string frame = Framing(payload);
//                 EV << frame;
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
                std::cout<<"Handling after NACK/TIMEOUT = "<< simTime().dbl()<<" payload = "<<payload<<endl;
                mmsg->setHeader(mmsg->getHeader());
//                EV << identifier << 3<< endl;
//                EV << payload << endl;
                std::string frame = Framing(payload);
//                EV << frame;
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
            std::cout<<"I am sender NOT SELF MSG handle ack/nack at = "<<simTime().dbl()<<" startWindowIndex = " <<startWindowIndex<<" currentWindowIndex = "<<currentWindowIndex<<" endWindowIndex= "<<endWindowIndex<<endl;
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
        std::cout<<"I am reciever = "<<simTime().dbl()<<endl;
        // receiving from sender message
        receivePacket(mmsg);
    }
}
void Node::handleACK(MyCustomMsg_Base* msg)
{
    int frame_number = ((msg->getAck_Nack_Num() + WS))%(WS + 1);
    std::cout<<"In handle ACK frame_number = "<<frame_number<<" msg->getAck_Nack_Num() = "<<msg->getAck_Nack_Num()<<endl;
    while(checkSeqBetween(startWindowIndex, currentWindowIndex, frame_number))
    {
//        EV <<"handleACK"<<endl;
        std::cout<<"In WHILE LOOP  startWindowIndex= "<<startWindowIndex<<" currentWindowIndex = "<<currentWindowIndex<<endl;

        std::cout<<"Handling ACK at time = "<<simTime().dbl()<<" startWindowIndex = " <<startWindowIndex<<" currentWindowIndex = "<<currentWindowIndex<<" endWindowIndex= "<<endWindowIndex<<endl;
        if (Timers[startWindowIndex] != NULL)
        {
            if(Timers[startWindowIndex]->isScheduled()) // check if timer is scheduled
            {
               EV<<"Stopped timer for frame "<<startWindowIndex<<endl; // if scheduled, cancel it
               cancelAndDelete(Timers[startWindowIndex]); // delete the timer message
               Timers[startWindowIndex] = NULL;
            }
        }
        startWindowIndex = incrementWindowNo(startWindowIndex);
        endWindowIndex = incrementWindowNo(endWindowIndex);
    }
    MyCustomMsg_Base* selfMessage2 = new MyCustomMsg_Base("self message");
    selfMessage2->setKind(0);
    scheduleAt(simTime(),selfMessage2);
}
void Node::handleNACK(MyCustomMsg_Base* msg)
{
    int counter = 1;
//       EV <<"handleNACK"<<endl;
//    currentWindowIndex = msg->getHeader();
    std::cout<<"I am handling nack at time = "<<simTime().dbl()<<endl;
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
        scheduleAt(simTime()+counter*PT+0.001,selfMessage);
        if (Timers[index] != nullptr){
            if(Timers[index]->isScheduled()) // check if timer is scheduled
             {
                cancelAndDelete(Timers[index]);
                Timers[index] = NULL;
             }
        }
        index = incrementWindowNo(index);
        counter++;
    }
    MyCustomMsg_Base* selfMessage2 = new MyCustomMsg_Base("self message");
    selfMessage2->setKind(0);
    scheduleAt(simTime()+counter*PT+0.001,selfMessage2);
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
    double random = uniform(0,1);
//    std::cout<<"random =  "<<random<<endl;
    bool lostACK = random<LP? true:false;
    std::cout<<"I am in recieve packet at time = "<<simTime().dbl()<<" Loss = "<<lostACK<<endl;
    if (seqNumToReceive == msg->getHeader())
    {
        // receiving from sender message
        std::string frame = msg->getPayload(); // message.getpayload
        char trailer = msg->getTrailer(); // message.gettrailer
//        std::cout << "Frame in reciever = "<<frame<<endl;
        bool errored_frame = ErrorDetection(frame, trailer);
        std::cout<<"I am the expected packet to receive= "<<seqNumToReceive<<" error frame = "<<errored_frame<<endl;
        if (errored_frame)
        {
            std::cout<<"Will I send NACK lastNACKedFrame= "<<lastNACKedFrame<<endl;
            if(lastNACKedFrame != seqNumToReceive)
            {
                // send Nack to sender with same header with probability LP
                std::cout<<"Frame recieved = "<<frame<<endl;
                if(!lostACK)
                {
                    msg->setAck_Nack_Num(seqNumToReceive);
                    msg->setFrame_Type(0);
                    sendDelayed(msg, TD+PT,"out");
                    EV << "Receiver: error in frame no"<< seqNumToReceive<<endl;
                    std::string logs = "At time["+std::to_string(simTime().dbl()) +"], Node["+std::to_string(getIndex())+"] Sending [NACK] with number ["+
                                                  std::to_string(seqNumToReceive)+"] , loss [No]";
                    logStates(logs);
                    lastNACKedFrame = seqNumToReceive;
                }
                else
                {
                    std::cout<<"********************************************************"<<endl;
                    EV << "Receiver: error in frame no and NACK is Lost"<< seqNumToReceive<<endl;
                    std::string logs = "At time["+std::to_string(simTime().dbl()) +"], Node["+std::to_string(getIndex())+"] Sending [NACK] with number ["+
                                                  std::to_string(seqNumToReceive)+"] , loss [Yes]";
                    logStates(logs);
                    lastNACKedFrame = seqNumToReceive;
                }
               return;
            }
        }
        else
        {
            int previousSeqNum  = seqNumToReceive;
//            std::cout <<"seqNumToReceive= " << seqNumToReceive << "WS = " << WS << endl;
            seqNumToReceive = incrementWindowNo(seqNumToReceive);
//            std::cout <<"seqNumToReceive= " << seqNumToReceive << endl;
            std::string payload = Deframing(frame);
            std::cout<<"I am the expected packet to receive= "<<seqNumToReceive<<" payload = "<<payload<<" sending ack"<<endl;

            if(!lostACK)
            {
                msg->setFrame_Type(1);
                msg->setAck_Nack_Num(seqNumToReceive);
                sendDelayed(msg, TD+PT,"out");
                EV << "Receiver: message received "<< payload<<endl;

                // print payload
                std::string logs = "At time["+std::to_string(simTime().dbl()) +"], Node["+std::to_string(getIndex())+"] Sending [ACK] with number ["+
                                                          std::to_string(seqNumToReceive)+"] , loss [No]";
                logStates(logs);
                // print payload
                logs = "Uploading payload=["+payload+"] and seq_num =["+std::to_string(previousSeqNum)+"] to the network layer";
                logStates(logs);
                lastNACKedFrame = -1;
            }
            else
            {
                std::cout<<"********************************************************"<<endl;
                std::string logs = "At time["+std::to_string(simTime().dbl()) +"], Node["+std::to_string(getIndex())+"] Sending [ACK] with number ["+
                                                                          std::to_string(seqNumToReceive)+"] , loss [Yes]";
                logStates(logs);
                // print payload
                logs = "Uploading payload=["+payload+"] and seq_num =["+std::to_string(previousSeqNum)+"] to the network layer";
                logStates(logs);
            }
            return;
        }
    }
    std::cout<<"I am not the expected packet to receive= "<<seqNumToReceive<<" while it came= "<<msg->getHeader()<<" payload = "<<msg->getPayload()<<" resesending ack"<<endl;
        msg->setFrame_Type(1);
        msg->setAck_Nack_Num(seqNumToReceive);
        sendDelayed(msg, TD+PT,"out");
//        EV << "Receiver: resnd ack "<<endl;

        // print payload
        std::string logs = "At time["+std::to_string(simTime().dbl()) +"], Node["+std::to_string(getIndex())+"] Resending [ACK] with number ["+
                                                  std::to_string(seqNumToReceive)+"] , loss [No]";
        logStates(logs);
}
std::string Node::Modification(std::string message, int& errorBit)
{
    int message_length = message.size();
    std::vector<std::bitset<8> >message_bit_stream;
    for(int i=0; i<message_length;i++)
    {
        std::bitset<8> bit_message(message[i]);
        message_bit_stream.push_back(bit_message);
    }
    errorBit = int(uniform(0, message.size()*8));
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
//    std::cout<<"Frame = "<<frame<<endl;
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
    int counter = 1;
//    EV <<"timeOutHandling"<<endl;
    std::string myMessage = "timeout for message" + myBuffer[startWindowIndex].second;
   MyCustomMsg_Base* selfMessage = new MyCustomMsg_Base(myMessage.c_str());
   selfMessage->setKind(2);
   selfMessage->setHeader(startWindowIndex);
   Timers[startWindowIndex] = NULL;
   scheduleAt(simTime(),selfMessage);
   int index = incrementWindowNo(startWindowIndex);
   std::string log = "Time out event at time [ "+std::to_string(simTime().dbl())+" ], at Node[ "+std::to_string(getIndex())+" ] for frame with seq_num=[ "+std::to_string(startWindowIndex)+" ]";
   logStates(log);
   while(index!=currentWindowIndex)
   {
       myMessage = "timeout for message" + myBuffer[index].second;
       MyCustomMsg_Base* selfMessage = new MyCustomMsg_Base(myMessage.c_str());
       selfMessage->setKind(3);
       selfMessage->setHeader(index);
       scheduleAt(simTime()+counter*PT,selfMessage);
       if (Timers[index] != nullptr){
           if(Timers[index]->isScheduled()) // check if timer is scheduled
            {
               cancelAndDelete(Timers[index]);
               Timers[index] = NULL;
            }
       }
       counter++;
       index = incrementWindowNo(index);
//       std::cout<<"index " <<index<<endl;
   }
   MyCustomMsg_Base* selfMessage2 = new MyCustomMsg_Base("self message after timeoutHandling to continue reading the file");
   selfMessage2->setKind(0);
   scheduleAt(simTime()+counter*PT,selfMessage2);
}


void Node::checkCases(const std::string& identifier,MyCustomMsg_Base* msg,std::string frame)
{
    unsigned int inputValue = std::bitset<4>(identifier).to_ulong();
    std::string modified_frame;
    std::string logs;
    // Switch-case logic based on the input value

    bool isModified = false;
        bool isLoss = false;
        bool isDuplicate = false;
        bool isDelayed = false;
        int errorBit;
        msg->setPayload(frame.c_str());
        switch (inputValue) {
            case 0b0000:
                msg->setPayload(frame.c_str());
                sendDelayed(msg, PT+TD,"out");
                break;
            case 0b0001: //delay
                isDelayed = true;
                msg->setPayload(frame.c_str());
                sendDelayed(msg, PT+TD+ED, "out");
                break;
            case 0b0010: //isDuplicate
                isDuplicate = true;
                msg->setPayload(frame.c_str());
                sendDelayed(msg, PT+TD, "out");
                sendDelayed(new MyCustomMsg_Base(*msg), PT+TD+DD, "out");
                break;
            case 0b0011: //duplicate,delay
                isDuplicate = true;
                isDelayed = true;
                msg->setPayload(frame.c_str());
                sendDelayed(msg, PT+TD+ED, "out");
                sendDelayed(new MyCustomMsg_Base(*msg), PT+TD+DD+ED, "out");
                break;
            case 0b0100: // loss
                isLoss = true;
                break;
            case 0b0101://loss,delay
                isLoss = true;
                isDelayed = true;
                break;
            case 0b0110://loss,duplicate
                isDuplicate = true;
                isLoss = true;
                break;
            case 0b0111://loss,duplicate,delay
                isDuplicate = true;
                isLoss = true;
                isDelayed = true;
                break;
            case 0b1000:
                isModified = true;
                modified_frame = Modification(frame,errorBit);
                msg->setPayload(modified_frame.c_str());
                sendDelayed(msg, PT+TD, "out");
                break;
            case 0b1001:
                isModified = true;
                isDelayed = true;
                modified_frame = Modification(frame,errorBit);
                msg->setPayload(modified_frame.c_str());
                sendDelayed(msg, PT+TD+ED, "out");
                break;
            case 0b1010:
                isModified = true;
                isDuplicate = true;
                modified_frame = Modification(frame,errorBit);
                msg->setPayload(modified_frame.c_str());
                sendDelayed(msg, PT+TD, "out");
                sendDelayed(new MyCustomMsg_Base(*msg), PT+TD+DD, "out");
                break;
            case 0b1011: // Mod, dup,delay
                isModified = true;
                isDuplicate = true;
                isDelayed = true;
                modified_frame = Modification(frame,errorBit);
                msg->setPayload(modified_frame.c_str());
                sendDelayed(msg, PT+TD+ED, "out");
                sendDelayed(new MyCustomMsg_Base(*msg), PT+TD+DD+ED, "out");
                break;
            case 0b1100: // loss, mod
                isModified = true;
                isLoss = true;
                modified_frame = Modification(frame,errorBit);
                msg->setPayload(modified_frame.c_str());
                break;
            case 0b1101: // loss, mod, delay
                isModified = true;
                isDelayed = true;
                isLoss = true;
                modified_frame = Modification(frame,errorBit);
                msg->setPayload(modified_frame.c_str());
                break;
            case 0b1110: // loss, mod, dup
                isModified = true;
                isDuplicate = true;
                isLoss = true;
                modified_frame = Modification(frame,errorBit);
                msg->setPayload(modified_frame.c_str());
                break;
            case 0b1111: // loss, mod, dup, delay
                isModified = true;
                isDuplicate = true;
                isDelayed = true;
                isLoss = true;
                modified_frame = Modification(frame,errorBit);
                msg->setPayload(modified_frame.c_str());
                break;

            default: break;
        }
        logs = "At time [ " + std::to_string((simTime() + PT).dbl()) + " ], Node[ " + std::to_string(getIndex()) + " ]"
            " [sent] frame with seq_num=[" + std::to_string(msg->getHeader()) + "] and payload=[ " + msg->getPayload() + " ] "
            "and trailer=[ " + std::bitset<8>(msg->getTrailer()).to_string() + " ] , Modified [ " + ((isModified) ? std::to_string(errorBit) : "-1") + " ]"
            " , Lost[ " + ((isLoss) ? "Yes" : "No") + " ], Duplicate [ " + ((isDuplicate) ? "1" : "0") + " ],"
            " Delay [ " + ((isDelayed) ? std::to_string(ED) : "0") + " ]";
        logStates(logs);
        if(isDuplicate)
        {
            logs = "At time [ " + std::to_string((simTime() + PT + DD).dbl()) + " ], Node[ " + std::to_string(getIndex()) + " ]"
                        " [sent] frame with seq_num=[" + std::to_string(msg->getHeader()) + "] and payload=[ " + msg->getPayload() + " ] "
                        "and trailer=[ " + std::bitset<8>(msg->getTrailer()).to_string() + " ] , Modified [ " + ((isModified) ? std::to_string(errorBit) : "-1") + " ]"
                        " , Lost[ " + ((isLoss) ? "Yes" : "No") + " ], Duplicate [ 2 ],"
                        " Delay [ " + ((isDelayed) ? std::to_string(ED) : "0") + " ]";
                    logStates(logs);
        }
    }


void Node::logStates(std::string logs)
{
    // Open the file in append mode
        std::ofstream outputFile(outputFileName, std::ios::app);

        // Check if the file is successfully opened
        if (!outputFile.is_open()) {
            std::cerr << "Error opening the file!" << std::endl;
        }

        outputFile << logs << std::endl;

        // Close the file
        outputFile.close();

//        std::cout << "String appended to the file successfully." << std::endl;
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
    EV <<endl<< " start = " << start << " end = " <<end<<" frame = "<<seq<<endl;
    if( ((start<= seq )&& (seq < end)) || ((end < start )&& (start <= seq)) // start<= seq 2 are deleted
            || ((seq < end )&& (end < start)))
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool Node::checkSeqBetween2(int start,int end,int seq)
{
    EV <<endl<< " start = " << start << " end = " <<end<<" frame = "<<seq<<endl;
    if( ((start<= seq )&& (seq < end)) || ((end < start )&& (start <= seq)) // start<= seq 2 are deleted
            || ((seq < end )&& (end < start)) || (seq == end))
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

void Node::printBuffer()
{
   EV<<"startWindowIndex = "<<myBuffer[startWindowIndex].second<<endl;
   EV<<"currentWindowIndex = "<<myBuffer[currentWindowIndex].second<<endl;
   EV<<"endWindowIndex = "<<myBuffer[endWindowIndex].second<<endl;
}

