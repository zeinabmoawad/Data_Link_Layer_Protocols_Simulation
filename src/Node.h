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
        std::pair<std::string, std::string>* myBuffer;


    protected:
        virtual void initialize();
        virtual void handleMessage(cMessage *msg);
        std::string Modification(std::string message);
        std::string Framing(std::string message);
        std::bitset<8> Checksum(std::string frame);
        std::string Deframing(std::string frame);
        bool ErrorDetection(std::string frame, char parity_byte);
        std::ifstream openFile(const std::string& fileName);
        std::pair<std::string, std::string> readNextLine(std::ifstream& file);
        void receivePacket(MyCustomMsg_Base* msg);
        void timeOutHandling();
        void checkCases(const std::string& cases,MyCustomMsg_Base* msg);
        void incrementSequenceNo()
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
        int incrementWindowNo(int number)
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
        bool checkSeqBetween(int start,int end,int seq)
        {
            if( ((start<= seq )&& (seq < end)) || ((end < start )&& (start <= seq))
                    || ((seq < end )&& (end < start))  )
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        bool checkCoordinator(MyCustomMsg_Base* msg)
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
                    WS = 1;

                }
                return true;
            }
            return false;
        }
};

#endif
