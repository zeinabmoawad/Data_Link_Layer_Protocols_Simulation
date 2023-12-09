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

#ifndef __NETWORK_PROJECR_NODE_H_
#define __NETWORK_PROJECR_NODE_H_

#include <omnetpp.h>
#include "MyCustomMsg_m.h"
#include <string>
#include <fstream>
#include <vector>
#include <bitset>


using namespace omnetpp;

/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
  // define data members of node
    public:
        int WS = 0;
        double TO = 0;
        double PT = 0;
        double TD = 0;
        double ED = 0;
        double DD = 0;
        double LP = 0;
        bool isSending = false;

        // sender parameters
        double startTime = 0;
        int startWindowIndex = 0;
        int endWindowIndex = 0;
        int currentWindowIndex =0;
        int bufferIndex = 0;
        std::ifstream file;

        // receiver parameters
        int seqNumToReceive = 0;
        std::vector<std::pair<std::string, std::string>> myBuffer;
        std::vector<MyCustomMsg_Base *> Timers;
        int lastNACKedFrame = -1;
        std::string outputFileName;

    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage *msg);
        std::string Modification(std::string message, int& errorBit);
        std::string Framing(std::string message);
        std::bitset<8> Checksum(std::string frame);
        std::string Deframing(std::string frame);
        bool ErrorDetection(std::string frame, char parity_byte);
        std::ifstream openFile(const std::string& fileName);
        std::pair<std::string, std::string> readNextLine(std::ifstream& file);
        void receivePacket(MyCustomMsg_Base* msg);
        void timeOutHandling();
        void handleACK(MyCustomMsg_Base* msg);
        void checkCases(const std::string& identifier,MyCustomMsg_Base* msg,std::string frame);
        void logStates(std::string logs);
        void handleNACK(MyCustomMsg_Base* msg);
        void incrementSequenceNo();
        int incrementWindowNo(int number);
        bool checkSeqBetween(int start,int end,int seq);
        bool checkCoordinator(MyCustomMsg_Base* msg);
};

#endif
