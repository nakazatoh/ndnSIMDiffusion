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

//int F_pitsize =0;
std::fstream file;

void
PeriodicStatsPrinter (Ptr<Node> node, Time next)
{
  Ptr<ndn::Pit> pit = node->GetObject<ndn::Pit> ();
  int pitsize;  
  pitsize = pit -> GetSize();

  /*if(node -> GetId() == 18) F_pitsize = 0;

  int pitsize,pitsizedif;
  pitsize = pit -> GetSize();
  pitsizedif = pitsize - F_pitsize;
  F_pitsize = pitsize;*/

  /*
  std::cout << Simulator::Now ().ToDouble (Time::S) << "\t"
            << node->GetId () << "\t"
            << Names::FindName (node) << "\t"
            << pitsize << "\t"
            << pitsizedif <<"\n";
  */

  //191015 LEE write PITsize to file
  //std::fstream file;
  file.open("210331_pitsize-congestion-topo-liner-dfcc-2src-ver2.txt",std::ios::out|std::ios::app);
  file << Simulator::Now ().ToDouble (Time::S) << "\t"
       << node->GetId () << "\t"
       << Names::FindName (node) << "\t"
       << pitsize << "\n";
  file.close();
  //


  Simulator::Schedule (next, PeriodicStatsPrinter, node, next);
}



int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  AnnotatedTopologyReader topologyReader ("", 25);
  //topologyReader.SetFileName ("src/ndnSIM/examples/topologies/topo-dfcc1-node-2src.txt");
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/topo-dfcc1-node-2src-bottleneck-ver2.txt");
  topologyReader.Read ();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute::PerOutFaceLimits","Limit","ns3::ndn::Limits::Rate");
  ndnHelper.EnableLimits(true, Seconds(0.2),0,1250);
  ndnHelper.SetContentStore ("ns3::ndn::cs::Lru",
                              "MaxSize", "0");
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
  consumerHelper.SetAttribute ("Frequency", StringValue ("200")); // 200 interests a second

  // on the first consumer node install a Consumer application
  // that will express interests in /dst1 namespace
  consumerHelper.SetPrefix ("/dst1");
  consumerHelper.Install (consumer1);

  // on the second consumer node install a Consumer application
  // that will express interests in /dst2 namespace

  //lee2005
  //consumerHelper.SetAttribute ("Frequency", StringValue ("20"));
  //
  consumerHelper.SetPrefix ("/dst2");
  consumerHelper.Install (consumer2);

  //lee200812 setting src2 to start at third second
  //ApplicationContainer consumer = consumerHelper.Install (consumer2);
  //consumer.Start(Seconds(3));
  //
  
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
            << "NodeId" << "\t"
            << "NodeName" << "\t"
            << "NumberOfPitEntries" << "\t"
            << "pitsize diff" << "\n";
  */

  //191112 write pitsize result to file 
  file.open("210331_pitsize-congestion-topo-liner-dfcc-2src-ver2.txt",std::ios::out);
  file << "Time" << "\t"
       << "NodeId" << "\t"
       << "NodeName" << "\t"
       << "NumberOfPITEntries" << "\t"
       << "PITsize diff" <<"\n";
  file.close();
  //


  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Dst1"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Dst2"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr17"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr16"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr15"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr14"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr13"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr12"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr11"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr10"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr9"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr8"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr7"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr6"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr5"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr4"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr3"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr2"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr1"), Seconds (0.1));
  /*Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr25"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr24"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr23"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr22"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr21"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr20"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr19"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr18"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Rtr17"), Seconds (0.1));*/
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Src2"), Seconds (0.1));
  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Src1"), Seconds (0.1));
  

//  Simulator::Schedule (Seconds (0), PeriodicStatsPrinter, Names::Find<Node>("Dst2"), Seconds (0.01));



  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes ();

  Simulator::Stop (Seconds (8.0));

  L2RateTracer::InstallAll ("drop-trace.txt", Seconds (0.1));
  ndn::L3RateTracer::InstallAll("210331_rate-trace-congestion-topo-liner-dfcc-2src-ver2.txt",Seconds (0.1));
  ndn::L3AggregateTracer::InstallAll("210331_aggregate-trace-congestion-topo-liner-dfcc-2src-ver2.txt",Seconds (0.1));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
