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
    std::string msgStr = "";
    msgStr +=  std::to_string(nodeStarting);
    msgStr += " ";
    msgStr +=  std::to_string(startingTime);

    cMessage *msgToSend1 = new cMessage(msgStr.c_str());
    cMessage *msgToSend2 = new cMessage(msgStr.c_str());
    send(msgToSend1, "out0");
    send(msgToSend2, "out1");



}

void Coordinator::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
