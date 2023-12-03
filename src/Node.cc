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
    myBuffer = new std::pair<std::string, std::string>[WS];

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
            if(msg->getKind() == 0)
            {
                EV << "Node "<< getIndex() << " is sender"<<endl;
                std::pair<std::string, std::string> line = readNextLine(file);
                std::string identifier = line.first;
                std::string payload = line.second;
                if (identifier.empty() && payload.empty())
                {
                    EV << "File reading is done is done";
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
                    scheduleAt(simTime()+PT+TO,msg);

                 }
            }
            else
            {
                // time out
                timeOutHandling();
                cancelAndDelete(mmsg);
            }
            return;

        }
        else{
            // Ack/Nack

        }

    }
    else
    {
        // receiving from sender message
        receivePacket(mmsg);
    }
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
        bool errored_frame = ErrorDetection(frame, trailer);
        if (errored_frame)
        {
            // send Nack to sender with same header with probability LP
            msg->setAck_Nack_Num(0);
            send(msg,"out");

        }
        else
        {
            std::string payload = Deframing(frame);
            msg->setAck_Nack_Num(1);
            send(msg,"out");
            // print payload

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
        std::cout<<bit_message<<endl;
    }
    int errorBit = int(uniform(0, message.size()*8));
    int errorChar = errorBit/8;
    message_bit_stream[errorChar][errorBit%8] = !message_bit_stream[errorChar][errorBit%8];
    std::cout<<"Error in " << "char: " <<errorChar << " in bit: " << errorBit%8 <<endl;
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
    for(int i=1;i<frame.size()-1;i++)
    {
        if(frame[i] == '\\')
        {
            i++;
        }
        payload += frame[i];
    }
    return payload;
}
bool Node::ErrorDetection(std::string frame, char parity_byte)
{
    return Checksum(frame) == std::bitset<8>(parity_byte);
}
void Node::timeOutHandling()
{
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
    switch (inputValue) {
        case 0b0000:
            scheduleAt(simTime()+PT,new cMessage("self message"));
            sendDelayed(msg, simTime()+PT+TD,"out");
            break;
        case 0b0001:
            msg->setPayload(frame.c_str());
            scheduleAt(simTime()+PT,new cMessage("self message"));
            sendDelayed(msg, simTime()+PT+TD+ED, "out");
            break;
        case 0b0010:
            msg->setPayload(frame.c_str());
            scheduleAt(simTime()+PT,new cMessage("self message"));
            sendDelayed(msg, simTime()+PT+TD, "out");
            sendDelayed(msg, simTime()+PT+TD+DD, "out");
            break;
        case 0b0011:
            msg->setPayload(frame.c_str());
            scheduleAt(simTime()+PT,new cMessage("self message"));
            sendDelayed(msg, simTime()+PT+TD+ED, "out");
            sendDelayed(msg, simTime()+PT+TD+DD+ED, "out");
            break;
        case 0b0100: // loss
            scheduleAt(simTime()+PT,new cMessage("self message"));
            break;
        case 0b0101:
            scheduleAt(simTime()+PT,new cMessage("self message"));
            break;
        case 0b0110:
            scheduleAt(simTime()+PT,new cMessage("self message"));
            break;
        case 0b0111:
            scheduleAt(simTime()+PT,new cMessage("self message"));
            break;
        case 0b1000:
            scheduleAt(simTime()+PT,new cMessage("self message"));
            modified_frame = Modification(frame);
            sendDelayed(msg, simTime()+PT+TD, "out");
            break;
        case 0b1001:
            scheduleAt(simTime()+PT,new cMessage("self message"));
            modified_frame = Modification(frame);
            sendDelayed(msg, simTime()+PT+TD+ED, "out");
            break;
        case 0b1010:
            scheduleAt(simTime()+PT,new cMessage("self message"));
            modified_frame = Modification(frame);
            sendDelayed(msg, simTime()+PT+TD, "out");
            sendDelayed(msg, simTime()+PT+TD+DD, "out");
            break;
        case 0b1011: // Mod, dup,delay
            scheduleAt(simTime()+PT,new cMessage("self message"));
            modified_frame = Modification(frame);
            sendDelayed(msg, simTime()+PT+TD+ED, "out");
            sendDelayed(msg, simTime()+PT+TD+DD+ED, "out");
            break;
        case 0b1100: // loss, mod
            scheduleAt(simTime()+PT,new cMessage("self message"));
            break;
        case 0b1101: // loss, mod, delay
            scheduleAt(simTime()+PT,new cMessage("self message"));
            break;
        case 0b1110: // loss, mod, dup
            scheduleAt(simTime()+PT,new cMessage("self message"));
            break;
        case 0b1111: // loss, mod, dup, delay
            scheduleAt(simTime()+PT,new cMessage("self message"));
            break;
        default: break;
    }

}
