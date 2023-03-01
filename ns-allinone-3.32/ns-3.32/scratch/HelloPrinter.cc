#include "ns3/core-module.h"

using namespace ns3;

void HelloPrinter () {
std::cout <<"At " << Now().GetSeconds() << " : Hello World" << std::endl; Simulator::Schedule (Seconds(1), &HelloPrinter);
}

int main (int argc, char *argv[]) {

CommandLine cmd; cmd.Parse (argc, argv);
//Schedule first call to HelloPrinter
Simulator::Schedule (Seconds(0), &HelloPrinter);
//Schedule simulator stop time after 10 seconds of starting.
Simulator::Stop (Seconds (10));
Simulator::Run (); Simulator::Destroy();

}

