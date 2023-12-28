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
    myBuffer.resize(WS + 1);
    Timers.resize(WS + 1);
    endWindowIndex = WS - 1;
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
    bool isCoordinator = checkCoordinator(mmsg);
    std::string identifier;
    std::string payload;
    int index = -1;
    if (isCoordinator)
    {

        // start sending if sender else do nothing
        if (isSending)
        {

            file = openFile("../input" + std::to_string(getIndex()) + ".txt");
            std::ifstream file(outputFileName);
            // clear file
            if (file.good())
            { // Check if the file exists
                file.close();
                std::ofstream ofs(outputFileName, std::ofstream::out | std::ofstream::trunc);
                ofs.close();
            }
            msg->setKind(0);
            scheduleAt(simTime() + startTime, msg);
            EV << "hereeeeee";
        }
    }
    // check i am sender or receive
    else if (isSending)
    {
        // sending
        EV << " i came here";
        if (msg->isSelfMessage())
        {
            MyCustomMsg_Base *send_msg = new MyCustomMsg_Base("message to be sent");
            EV << "My type is = " << msg->getKind();
            bool resend = false;
            bool isSend = false;
            if (msg->getKind() == 0 && checkToContinueReading(startWindowIndex, endWindowIndex, currentWindowIndex))
            {
                std::pair<std::string, std::string> line = readNextLine(file);
                identifier = line.first;
                payload = line.second;
                if (identifier.empty() && payload.empty())
                {
                    EV << "File reading is done is done";
                }
                else
                {
                    send_msg->setHeader(currentWindowIndex);
                    myBuffer[currentWindowIndex] = line;
                    index = currentWindowIndex;
                    resend = true;
                    incrementSequenceNo();
                    std::cout<<"sending at "<<simTime().dbl()<<" "<<currentWindowIndex<<endl;
                }
            }
            else if (msg->getKind() == 1)
            {
                handleNack_Timeout(-1);
                // timeOutHandling();
                isSend = true;
                return;
            }
            else if (msg->getKind() == 2) // NACK
            {
                payload = myBuffer[mmsg->getHeader()].second;
                identifier = "0000";
                send_msg->setHeader(mmsg->getHeader());
                index = mmsg->getHeader();
                isSend = true;
            }
            else if (msg->getKind() == 3) // normal case
            {
                payload = myBuffer[mmsg->getHeader()].second;
                identifier = myBuffer[mmsg->getHeader()].first;
                send_msg->setHeader(mmsg->getHeader());
                index = mmsg->getHeader();
                isSend = true;
            }
            if (isSend || resend)
            {
                std::string logs = "At time[" + std::to_string(simTime().dbl()) + "], Node[" + std::to_string(getIndex()) + "], Introducing channel error with code = [ " + identifier + " ]";
                EV << logs << endl;
                logStates(logs);
                std::string frame = Framing(payload);
                std::bitset<8> parity_byte = Checksum(frame);
                char trailer = static_cast<char>(parity_byte.to_ulong());
                send_msg->setTrailer(trailer);
                send_msg->setFrame_Type(2);
                // Switch cases on identifier
                checkCases(identifier, send_msg, frame);
                if (resend)
                {
                    MyCustomMsg_Base *selfMessage = new MyCustomMsg_Base("self message1");
                    selfMessage->setKind(0);
                    scheduleAt(simTime() + PT, selfMessage);
                }
                if (Timers[index] != nullptr)
                {
                    if (Timers[index]->isScheduled()) // check if timer is scheduled
                    {
                        cancelAndDelete(Timers[index]); // delete the timer message
                        Timers[index] = NULL;
                    }
                }
                Timers[index] = new MyCustomMsg_Base("time out");
                Timers[index]->setKind(1);
                scheduleAt(simTime() + PT + TO, Timers[index]);


            }
        }
        else
        {
            // Ack/Nack
            if (mmsg->getFrame_Type() == 1)
            {
                handleACK(mmsg);
            }
            else if (mmsg->getFrame_Type() == 0)
            {
                handleNack_Timeout(mmsg->getAck_Nack_Num(), true);
            }
        }
    }
    else
    {
        // receiving from sender message
        receivePacket(mmsg);
    }
    //    cancelAndDelete(mmsg);
}
void Node::handleACK(MyCustomMsg_Base *msg)
{
    int frame_number = ((msg->getAck_Nack_Num() + WS)) % (WS + 1);
    std::cout<<msg->getPayload()<<"   "<<simTime().dbl()<<"  "<<frame_number<<"  "<<startWindowIndex<<"  "<<currentWindowIndex<<"  "<<checkSeqBetween(startWindowIndex, currentWindowIndex, frame_number)<<endl;
    while (checkSeqBetween(startWindowIndex, currentWindowIndex, frame_number))
    {
        if (Timers[startWindowIndex] != NULL)
        {
            if (Timers[startWindowIndex]->isScheduled()) // check if timer is scheduled
            {
                cancelAndDelete(Timers[startWindowIndex]); // delete the timer message
                Timers[startWindowIndex] = NULL;
            }
        }
//        myBuffer[startWindowIndex] = std::make_pair("-1","-1");
        startWindowIndex = incrementWindowNo(startWindowIndex);
        endWindowIndex = incrementWindowNo(endWindowIndex);
    }
    std::cout<<"At the end  "<<startWindowIndex<<"  "<<currentWindowIndex<<"  "<<endWindowIndex<<endl;
    MyCustomMsg_Base *selfMessage2 = new MyCustomMsg_Base("self message");
    selfMessage2->setKind(0);
    scheduleAt(simTime(), selfMessage2);
}

void Node::handleNack_Timeout(int frameNum, bool isNack)
{
    int counter = 1;
    if (isNack)
    {
        while (startWindowIndex != frameNum)
        {
            if (Timers[startWindowIndex] != NULL)
            {
                if (Timers[startWindowIndex]->isScheduled()) // check if timer is scheduled
                {
                    cancelAndDelete(Timers[startWindowIndex]); // delete the timer message
                    Timers[startWindowIndex] = NULL;
                }
            }
            startWindowIndex = incrementWindowNo(startWindowIndex);
            endWindowIndex = incrementWindowNo(endWindowIndex);
        }
    }
    else
    {
        std::string logs = "Time out event at time [ " + std::to_string(simTime().dbl()) + " ], at Node[ " + std::to_string(getIndex()) + " ] for frame with seq_num=[ " + std::to_string(startWindowIndex) + " ]";
        EV << logs << endl;
        logStates(logs);
    }
    MyCustomMsg_Base *selfMessage = new MyCustomMsg_Base("handling NACK/Timeout");
    selfMessage->setKind(2);
    selfMessage->setHeader(startWindowIndex);
    scheduleAt(simTime() + 0.001, selfMessage);
    if (Timers[startWindowIndex] != NULL)
    {
        if (Timers[startWindowIndex]->isScheduled()) // check if timer is scheduled
        {
            cancelAndDelete(Timers[startWindowIndex]);
            Timers[startWindowIndex] = NULL;
        }
    }
    int index = incrementWindowNo(startWindowIndex);
    while (index != currentWindowIndex)
    {
        MyCustomMsg_Base *selfMessage = new MyCustomMsg_Base("shandling NACK/Timeout");
        selfMessage->setKind(3);
        selfMessage->setHeader(index);
        scheduleAt(simTime() + counter * PT + 0.001, selfMessage);
        if (Timers[index] != nullptr)
        {
            if (Timers[index]->isScheduled()) // check if timer is scheduled
            {
                cancelAndDelete(Timers[index]);
                Timers[index] = NULL;
            }
        }
        index = incrementWindowNo(index);
        counter++;
    }
    MyCustomMsg_Base *selfMessage2 = new MyCustomMsg_Base("self message to continue reading the file");
    selfMessage2->setKind(0);
    scheduleAt(simTime() + counter * PT + 0.001, selfMessage2);
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
std::pair<std::string, std::string> Node::readNextLine(std::ifstream &file)
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
void Node::receivePacket(MyCustomMsg_Base *msg)
{
    double random = uniform(0, 1);
    bool lostACK = random < LP ? true : false;
    MyCustomMsg_Base *mmsg = new MyCustomMsg_Base("Receiver sent");
    std::cout<<simTime().dbl()<<"  seq expected = "<<seqNumToReceive<<" msg->getHeader() "<<msg->getHeader()<<"  "<<lostACK<<endl;
    mmsg->setPayload(msg->getPayload());
    if (seqNumToReceive == msg->getHeader())
    {
        // receiving from sender message
        std::string frame = msg->getPayload(); // message.getpayload
        char trailer = msg->getTrailer();      // message.gettrailer
        bool errored_frame = ErrorDetection(frame, trailer);
        std::string payload;
        int previousSeqNum;
        bool trial = true;
        std::cout<<errored_frame<<endl;
        if (errored_frame)
        {
            if (lastNACKedFrame != seqNumToReceive)
            {
                // send Nack to sender with same header with probability LP
                if (!lostACK)
                {
                    mmsg->setAck_Nack_Num(seqNumToReceive);
                    mmsg->setFrame_Type(0);
                    sendDelayed(mmsg, TD + PT, "out");
                    lastNACKedFrame = seqNumToReceive;
                    std::cout<<msg->getPayload()<<"   i didnt send anything"<<endl;
                }
                else
                {
                    lastNACKedFrame = seqNumToReceive;
                }
            }
            else
                trial = false;
        }
        else
        {
            previousSeqNum = seqNumToReceive;
            seqNumToReceive = incrementWindowNo(seqNumToReceive);
            payload = Deframing(frame);

            if (!lostACK)
            {
                std::cout<<"AT time   =  "<<simTime().dbl()<<"  "<<!lostACK<<endl;
                mmsg->setFrame_Type(1);
                mmsg->setAck_Nack_Num(seqNumToReceive);
                sendDelayed(mmsg, TD + PT, "out");
                lastNACKedFrame = -1;
            }
            std::cout<<"AT time   =  "<<simTime().dbl()<<"  "<<!lostACK<<endl;
        }
        if (trial)
        {
            if (!errored_frame)
            {
                std::string logs = "Uploading payload=[" + payload + "] and seq_num =[" + std::to_string(previousSeqNum) + "] to the network layer";
                EV << logs << endl;
                logStates(logs);
            }
            std::string logs = "At time[" + std::to_string(PT + simTime().dbl()) + "], Node[" + std::to_string(getIndex()) + "] Sending [" +
                               ((errored_frame) ? "NACK" : "ACK") + "] with number [" +
                               std::to_string(seqNumToReceive) + "] , loss [" +
                               ((!lostACK) ? "No" : "Yes") + "]";
            EV << logs << endl;
            logStates(logs);
            return;
        }
    }
    mmsg->setFrame_Type(1);
    mmsg->setAck_Nack_Num(seqNumToReceive);
    sendDelayed(mmsg, TD + PT, "out");
}
std::string Node::Modification(std::string message, int &errorBit)
{
    int message_length = message.size();
    std::vector<std::bitset<8>> message_bit_stream;
    for (int i = 0; i < message_length; i++)
    {
        std::bitset<8> bit_message(message[i]);
        message_bit_stream.push_back(bit_message);
    }
    errorBit = int(uniform(0, message.size() * 8));
    int errorChar = errorBit / 8;
    message_bit_stream[errorChar][errorBit % 8] = !message_bit_stream[errorChar][errorBit % 8];
    std::string errored_message = "";
    for (std::vector<std::bitset<8>>::iterator it = message_bit_stream.begin(); it != message_bit_stream.end(); ++it)
    {
        errored_message += (char)(*it).to_ulong();
    }

    return errored_message;
}
std::string Node::Framing(std::string message)
{
    std::string frame = "";
    for (int i = 0; i < message.size(); i++)
    {
        if (message[i] == '/' || message[i] == '$')
        {
            frame += '/';
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
    for (int i = 0; i < frame.size(); i++)
    {
        std::bitset<8> frame_byte(frame[i]);
        frame_int = (frame_byte).to_ulong();
        checksum += frame_int;
        if (checksum > 255)
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
    for (int i = 1; i < frame.size() - 1; i++)
    {
        if (frame[i] == '/')
        {
            i++; // Skip these characters
        }
        payload += frame[i];
    }
    return payload;
}
bool Node::ErrorDetection(std::string frame, char parity_byte)
{
    return Checksum(frame) != std::bitset<8>(parity_byte);
}

void Node::checkCases(const std::string &identifier, MyCustomMsg_Base *msg, std::string frame)
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
    switch (inputValue)
    {
    case 0b0000:
        msg->setPayload(frame.c_str());
        sendDelayed(msg, PT + TD, "out");
        break;
    case 0b0001: // delay
        isDelayed = true;
        msg->setPayload(frame.c_str());
        sendDelayed(msg, PT + TD + ED, "out");
        break;
    case 0b0010: // isDuplicate
        isDuplicate = true;
        msg->setPayload(frame.c_str());
        sendDelayed(msg, PT + TD, "out");
        sendDelayed(new MyCustomMsg_Base(*msg), PT + TD + DD, "out");
        break;
    case 0b0011: // duplicate,delay
        isDuplicate = true;
        isDelayed = true;
        msg->setPayload(frame.c_str());
        sendDelayed(msg, PT + TD + ED, "out");
        sendDelayed(new MyCustomMsg_Base(*msg), PT + TD + DD + ED, "out");
        break;
    case 0b0100: // loss
        isLoss = true;
        break;
    case 0b0101: // loss,delay
        isLoss = true;
        isDelayed = true;
        break;
    case 0b0110: // loss,duplicate
        isDuplicate = true;
        isLoss = true;
        break;
    case 0b0111: // loss,duplicate,delay
        isDuplicate = true;
        isLoss = true;
        isDelayed = true;
        break;
    case 0b1000:
        isModified = true;
        modified_frame = Modification(frame, errorBit);
        msg->setPayload(modified_frame.c_str());
        sendDelayed(msg, PT + TD, "out");
        break;
    case 0b1001:
        isModified = true;
        isDelayed = true;
        modified_frame = Modification(frame, errorBit);
        msg->setPayload(modified_frame.c_str());
        sendDelayed(msg, PT + TD + ED, "out");
        break;
    case 0b1010:
        isModified = true;
        isDuplicate = true;
        modified_frame = Modification(frame, errorBit);
        msg->setPayload(modified_frame.c_str());
        sendDelayed(msg, PT + TD, "out");
        sendDelayed(new MyCustomMsg_Base(*msg), PT + TD + DD, "out");
        break;
    case 0b1011: // Mod, dup,delay
        isModified = true;
        isDuplicate = true;
        isDelayed = true;
        modified_frame = Modification(frame, errorBit);
        msg->setPayload(modified_frame.c_str());
        sendDelayed(msg, PT + TD + ED, "out");
        sendDelayed(new MyCustomMsg_Base(*msg), PT + TD + DD + ED, "out");
        break;
    case 0b1100: // loss, mod
        isModified = true;
        isLoss = true;
        modified_frame = Modification(frame, errorBit);
        msg->setPayload(modified_frame.c_str());
        break;
    case 0b1101: // loss, mod, delay
        isModified = true;
        isDelayed = true;
        isLoss = true;
        modified_frame = Modification(frame, errorBit);
        msg->setPayload(modified_frame.c_str());
        break;
    case 0b1110: // loss, mod, dup
        isModified = true;
        isDuplicate = true;
        isLoss = true;
        modified_frame = Modification(frame, errorBit);
        msg->setPayload(modified_frame.c_str());
        break;
    case 0b1111: // loss, mod, dup, delay
        isModified = true;
        isDuplicate = true;
        isDelayed = true;
        isLoss = true;
        modified_frame = Modification(frame, errorBit);
        msg->setPayload(modified_frame.c_str());
        break;

    default:
        break;
    }
    logs = "At time [ " + std::to_string((simTime() + PT).dbl()) + " ], Node[ " + std::to_string(getIndex()) + " ]"
                                                                                                               " [sent] frame with seq_num=[" +
           std::to_string(msg->getHeader()) + "] and payload=[ " + msg->getPayload() + " ] "
                                                                                       "and trailer=[ " +
           std::bitset<8>(msg->getTrailer()).to_string() + " ] , Modified [ " + ((isModified) ? std::to_string(errorBit) : "-1") + " ]"
                                                                                                                                   " , Lost[ " +
           ((isLoss) ? "Yes" : "No") + " ], Duplicate [ " + ((isDuplicate) ? "1" : "0") + " ],"
                                                                                          " Delay [ " +
           ((isDelayed) ? std::to_string(ED) : "0") + " ]";
    EV << logs << endl;
    logStates(logs);
    if (isDuplicate)
    {
        logs = "At time [ " + std::to_string((simTime() + PT + DD).dbl()) + " ], Node[ " + std::to_string(getIndex()) + " ]"
                                                                                                                        " [sent] frame with seq_num=[" +
               std::to_string(msg->getHeader()) + "] and payload=[ " + msg->getPayload() + " ] "
                                                                                           "and trailer=[ " +
               std::bitset<8>(msg->getTrailer()).to_string() + " ] , Modified [ " + ((isModified) ? std::to_string(errorBit) : "-1") + " ]"
                                                                                                                                       " , Lost[ " +
               ((isLoss) ? "Yes" : "No") + " ], Duplicate [ 2 ],"
                                           " Delay [ " +
               ((isDelayed) ? std::to_string(ED) : "0") + " ]";
        logStates(logs);
        EV << logs << endl;
    }
}

void Node::logStates(std::string logs)
{
    // Open the file in append mode
    std::ofstream outputFile(outputFileName, std::ios::app);

    // Check if the file is successfully opened
    if (!outputFile.is_open())
    {
        std::cerr << "Error opening the file!" << std::endl;
    }

    outputFile << logs << std::endl;

    // Close the file
    outputFile.close();
}
void Node::incrementSequenceNo()
{
    if (currentWindowIndex + 1 > WS)
    {
        currentWindowIndex = 0;
    }
    else
    {
        currentWindowIndex++;
    }
}
int Node::incrementWindowNo(int number)
{
    if (number + 1 > WS)
    {
        number = 0;
    }
    else
    {
        number++;
    }
    return number;
}
bool Node::checkSeqBetween(int start, int end, int seq)
{
    if (((start <= seq) && (seq < end)) || ((end < start) && (start <= seq)) // start<= seq 2 are deleted
        || ((seq < end) && (end < start)))
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool Node::checkToContinueReading(int start, int end, int seq)
{
    if (((start <= seq) && (seq < end)) || ((end < start) && (start <= seq)) // start<= seq 2 are deleted
        || ((seq < end) && (end < start)) || (seq == end))
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool Node::checkCoordinator(MyCustomMsg_Base *msg)
{
    int senderID = msg->getSenderModuleId();
    // check if coordinator ssending
    EV << "i am in checkCoordinator";
    if (senderID == 2)
    {
        // coordinator sending
        int nodeSending = msg->getAck_Nack_Num();
        if (nodeSending == getIndex())
        {
            // sender
            isSending = true;
            startTime = atoi(msg->getPayload());
            // set parameter of sender;
            EV << "iam " << getIndex() << " " << startTime << endl;
        }
        else
        {
            isSending = false;
        }
        return true;
    }
    return false;
}
