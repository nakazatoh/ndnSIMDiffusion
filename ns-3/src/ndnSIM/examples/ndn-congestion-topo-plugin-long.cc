/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2012 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */
// ndn-congestion-topo-plugin.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

using namespace ns3;

/**
 * This scenario simulates a grid topology (using topology reader module)
 *
 *   /------\	                                                 /------\
 *   | Src1 |<--+                                            +-->| Dst1 |
 *   \------/    \                                          /    \------/
 *            	 \                                        /     
 *                 +-->/------\   "bottleneck"  /------\<-+      
 *                     | Rtr1 |<===============>| Rtr2 |         
 *                 +-->\------/                 \------/<-+      
 *                /                                        \
 *   /------\    /                                          \    /------\
 *   | Src2 |<--+                                            +-->| Dst2 |
 *   \------/                                                    \------/
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-congestion-topo-plugin
 */



void
PeriodicStatsPrinter (Ptr<Node> node, Time next)
{
  Ptr<ndn::Pit> pit = node->GetObject<ndn::Pit> ();

  std::cout << Simulator::Now ().ToDouble (Time::S) << "\t"
            << node->GetId () << "\t"
            << Names::FindName (node) << "\t"
            << pit->GetSize () << "\n";
  
  Simulator::Schedule (next, PeriodicStatsPrinter, node, next);
}



int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/topo-long-node.txt");
  topologyReader.Read ();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute::PerOutFaceLimits","Limit","ns3::ndn::Limits::Rate");
  ndnHelper.EnableLimits(true, Seconds(0.2),0,1250);
  ndnHelper.SetContentStore ("ns3::ndn::cs::Lru",
                              "MaxSize", "10000");
  ndnHelper.InstallAll ();

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  // Getting containers for the consumer/producer
  Ptr<Node> consumer1 = Names::Find<Node> ("Src1");
  Ptr<Node> consumer2 = Names::Find<Node> ("Src2");

  Ptr<Node> producer1 = Names::Find<Node> ("Dst1");
  Ptr<Node> producer2 = Names::Find<Node> ("Dst2");

  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute ("Frequency", StringValue ("50")); // 100 interests a second

  // on the first consumer node install a Consumer application
  // that will express interests in /dst1 namespace
  consumerHelper.SetPrefix ("/dst1");
  consumerHelper.Install (consumer1);

  // on the second consumer node install a Consumer application
  // that will express interests in /dst2 namespace
  consumerHelper.SetPrefix ("/dst2");
  consumerHelper.Install (consumer2);
  
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1250"));  

  // Register /dst1 prefix with global routing controller and
  // install producer that will satisfy Interests in /dst1 namespace
  ndnGlobalRoutingHelper.AddOrigins ("/dst1", producer1);
  producerHelper.SetPrefix ("/dst1");
  producerHelper.Install (producer1);

  // Register /dst2 prefix with global routing controller and
  // install producer that will satisfy Interests in /dst2 namespace
  ndnGlobalRoutingHelper.AddOrigins ("/dst2", producer2);
  producerHelper.SetPrefix ("/dst2");
  producerHelper.Install (producer2);
/*
  std::cout << "Time" << "\t"
            << "NodeID" << "\t"
            << "FaceID" << "\t" // incomingDataFaceID
            << "PITsize" << "\t"
            << "F_PITsize" << "\t"
            << "F_Rate" << "\t"
            << "m_bucketLeak" << "\t"
            << "??????/Data" << "\t"
            << "RTT" << "\t"
            << "m_bucketMax" << "\n";
*/

  std::cout << "Time" << "\t"
            << "NodeId" << "\t"
            << "NodeName" << "\t"
            << "NumberOfPitEntries" << "\n";
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Src1"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr1"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr3"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr5"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr7"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr9"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr11"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr13"), Seconds (0.01));

  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Src2"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr2"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr4"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr6"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr8"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr10"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr12"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr14"), Seconds (0.01));

  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("RtrA"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("RtrB"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Dst1"), Seconds (0.01));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Dst2"), Seconds (0.01));




  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes ();

  Simulator::Stop (Seconds (10.0));

  L2RateTracer::InstallAll ("drop-trace-dfcc-long.txt", Seconds (0.01));
  ndn::L3RateTracer::InstallAll("rate-trace-congestion-topo-long-dfcc.txt",Seconds (0.01));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
