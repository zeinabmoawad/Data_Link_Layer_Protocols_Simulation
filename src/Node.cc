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
            std::pair<std::string, std::string> line = readNextLine(file);
            std::string identifier = line.first;
            std::string payload = line.second;
            if (identifier.empty() && payload.empty())
            {
                EV << "File reading is done is done";
            }
            else
            {
                EV << identifier << endl;
                EV << payload << endl;
                std::string frame = Framing(payload);
                EV << frame;
                std::bitset<8> parity_byte = Checksum(frame);
                char trailer = static_cast<char>(parity_byte.to_ulong());
                mmsg->setTrailer(trailer);
                // Switch cases on identifier
            }
        }

    }
    // check i am sender or receive
    else if(isSending)
    {
        // sending
        std::pair<std::string, std::string> line = readNextLine(file);
        std::string identifier = line.first;
        std::string payload = line.second;
        if (identifier.empty() && payload.empty())
        {
            EV << "File reading is done is done";
        }
        else
        {
            EV << identifier << endl;
            EV << payload << endl;
            std::string frame = Framing(payload);
            EV << frame;
            std::bitset<8> parity_byte = Checksum(frame);
            char trailer = static_cast<char>(parity_byte.to_ulong());
            mmsg->setTrailer(trailer);
            // Switch cases on identifier
            }


        }
    else
    {
        // receiving from sender message
        std::string frame = mmsg->getPayload(); // message.getpayload
        char trailer = mmsg->getTrailer(); // message.gettrailer
        std::string payload = Deframing(frame);
        bool errored_frame = ErrorDetection(frame, trailer);
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
        // check for error, call detection

        bool errorDetected = true;
        if (errorDetected)
        {
            // send Nack to sender

        }


    }
}
std::vector<std::bitset<8> > Node::Modification(std::string message)
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
    for(std::vector<std::bitset<8> >::iterator it = message_bit_stream.begin(); it != message_bit_stream.end(); ++it)
    {
        std::cout<<*it<<endl;
    }
    return message_bit_stream;
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
