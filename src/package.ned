package network_projecr;

@license(LGPL);
//
// TODO documentation
//
network Network
{
    parameters:
        int WS = default(3);
        double TO = default(10);
        double PT = default(0.5);
        double TD = default(1.0);
        double ED = default(4.0);
        double DD = default(0.1);
        double LP = default(0);
    @display("bgb=373,265");
    submodules:
//        node0: Node {
//            @display("p=82,168");
//        }
//        node1: Node {
//            @display("p=285,168");
//        }
        coordinator: Coordinator {
            @display("p=176,104");
        }
        
      	nodes[2]:Node;
    connections:
//        coordinator.out0 --> node0.coordinator_in;
//        coordinator.out1 --> node1.coordinator_in;
//        node0.out --> node1.in;
//        node1.out --> node0.in;
        coordinator.out0 --> nodes[0].coordinator_in;
        coordinator.out1 --> nodes[1].coordinator_in;
        nodes[0].out --> nodes[1].in;
        nodes[1].out --> nodes[0].in;
			
}

