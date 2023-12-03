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

#include "Coordinator.h"
#include <string>
#include <fstream>

#include "MyCustomMsg_m.h"
Define_Module(Coordinator);

void Coordinator::initialize()
{
    // TODO - Generated method body

    // read text file to get starting node and starting time

    // Open the file
    std::ifstream inputFile("../coordinator.txt");

    // Check if the file is opened successfully
    if (!inputFile.is_open()) {
        std::cerr << "Unable to open the file." << std::endl;
        return ; // Return an error code
    }
    std::string word;
    inputFile >> word;
    nodeStarting = atoi(word.c_str());
    inputFile >> word;
    startingTime = atoi(word.c_str());



    // send information to Nodes
    std::string msgStr = std::to_string(startingTime);
    MyCustomMsg_Base *msgToSend1 = new MyCustomMsg_Base();
    MyCustomMsg_Base *msgToSend2 = new MyCustomMsg_Base();
    msgToSend1->setAck_Nack_Num(nodeStarting);
    msgToSend2->setAck_Nack_Num(nodeStarting);
    msgToSend1->setPayload(msgStr.c_str());
    msgToSend2->setPayload(msgStr.c_str());
    send(msgToSend1, "out0");
    send(msgToSend2, "out1");



}

void Coordinator::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
