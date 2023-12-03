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

    EV << "WS = " << WS <<endl;
    EV << "TO = " << TO <<endl;
    EV << "PT = " << PT <<endl;
    EV << "LP = " << LP <<endl;
    EV << "TD = " << TD <<endl;
    EV << "ED = " << ED <<endl;
    EV << "DD = " << DD <<endl;



}

void Node::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    MyCustomMsg_Base *mmsg = check_and_cast<MyCustomMsg_Base *>(msg);
    int senderID = mmsg->getSenderModuleId();
    // check if coordinator ssending
    if (senderID ==2)
    {
        // coordinator sending
        EV << "coordinator sending "<<endl;
        int nodeSending = mmsg->getAck_Nack_Num();
        if(nodeSending == getIndex())
        {
            // sender
            isSending = true;
            startTime = atoi(mmsg->getPayload());
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
    }

}
