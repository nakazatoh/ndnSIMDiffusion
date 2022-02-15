/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
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
 * Author:  Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *          Ilya Moiseenko <iliamo@cs.ucla.edu>
 */

#include "ndn-forwarding-strategy.h"
///////////////////////////////////////////
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
///////////////////////////////////////////
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndn-content-store.h"
#include "ns3/ndn-face.h"

#include "ns3/assert.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"
#include "ns3/string.h"


#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
/////////////////////////////////////////////////////////////////
#include "ns3/ndnSIM/utils/ndn-fw-feedback-pitsize-tag.h"
#include "ns3/ndnSIM/utils/ndn-fw-feedback-pitsize-difference-tag.h"
#include "ns3/ndnSIM/utils/ndn-fw-feedback-rate-tag.h"
/////////////////////////////////////////////////////////////////
#include <boost/ref.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <iostream>

namespace ll = boost::lambda;

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ForwardingStrategy);

NS_LOG_COMPONENT_DEFINE (ForwardingStrategy::GetLogName ().c_str ());

std::string
ForwardingStrategy::GetLogName ()
{
  return "ndn.fw";
}

TypeId ForwardingStrategy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ForwardingStrategy")
    .SetGroupName ("Ndn")
    .SetParent<Object> ()

    ////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////

    .AddTraceSource ("OutInterests",  "OutInterests",  MakeTraceSourceAccessor (&ForwardingStrategy::m_outInterests))
    .AddTraceSource ("InInterests",   "InInterests",   MakeTraceSourceAccessor (&ForwardingStrategy::m_inInterests))
    .AddTraceSource ("DropInterests", "DropInterests", MakeTraceSourceAccessor (&ForwardingStrategy::m_dropInterests))

    ////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////

    .AddTraceSource ("OutData",  "OutData",  MakeTraceSourceAccessor (&ForwardingStrategy::m_outData))
    .AddTraceSource ("InData",   "InData",   MakeTraceSourceAccessor (&ForwardingStrategy::m_inData))
    .AddTraceSource ("DropData", "DropData", MakeTraceSourceAccessor (&ForwardingStrategy::m_dropData))

    ////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////

    .AddTraceSource ("SatisfiedInterests",  "SatisfiedInterests",  MakeTraceSourceAccessor (&ForwardingStrategy::m_satisfiedInterests))
    .AddTraceSource ("TimedOutInterests",   "TimedOutInterests",   MakeTraceSourceAccessor (&ForwardingStrategy::m_timedOutInterests))

    .AddAttribute ("CacheUnsolicitedDataFromApps", "Cache unsolicited data that has been pushed from applications",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ForwardingStrategy::m_cacheUnsolicitedDataFromApps),
                   MakeBooleanChecker ())
    
    .AddAttribute ("CacheUnsolicitedData", "Cache overheard data that have not been requested",
                   BooleanValue (false),
                   MakeBooleanAccessor (&ForwardingStrategy::m_cacheUnsolicitedData),
                   MakeBooleanChecker ())

    .AddAttribute ("DetectRetransmissions", "If non-duplicate interest is received on the same face more than once, "
                                            "it is considered a retransmission",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ForwardingStrategy::m_detectRetransmissions),
                   MakeBooleanChecker ())
    ;
  return tid;
}
///////////////////////////////////////////////
ForwardingStrategy::ForwardingStrategy ()
{
  ad = 5.0;
  DataPacketNum[20] = {};//LEE
}
//////////////////////////////////////////////
ForwardingStrategy::~ForwardingStrategy ()
{
}

void
ForwardingStrategy::NotifyNewAggregate ()
{
  if (m_pit == 0)
    {
      m_pit = GetObject<Pit> ();
    }
  if (m_fib == 0)
    {
      m_fib = GetObject<Fib> ();
    }
  if (m_contentStore == 0)
    {
      m_contentStore = GetObject<ContentStore> ();
    }

  Object::NotifyNewAggregate ();
}

void
ForwardingStrategy::DoDispose ()
{
  m_pit = 0;
  m_contentStore = 0;
  m_fib = 0;

  Object::DoDispose ();
}

void
ForwardingStrategy::OnInterest (Ptr<Face> inFace,
                                Ptr<Interest> interest)
{
        NS_LOG_FUNCTION (inFace << interest->GetName ());
        m_inInterests (interest, inFace);

///////////////////////////////////////////////////////////////////
  //  can see how to move packet
 /* Ptr<Node> node = inFace -> GetNode();
  uint32_t nodeID = node -> GetId();
  Ptr<Limits> faceLimits = inFace -> GetObject<Limits>();
  double rate = faceLimits -> GetCurrentLimit();
  
  std::cout << Simulator::Now ().ToDouble (Time::S) << "\t" //time
            << nodeID << "\t"
            << inFace -> GetId() << "\t"  // incomingDataFaceID
            << m_pit->GetSize() << "\t"
            << rate << "\t"
            << "comeInterest" << "\n";
  */
//////////////////////////////////////////////////////////////////
        Ptr<pit::Entry> pitEntry = m_pit->Lookup (*interest);
        bool similarInterest = true;

        if (pitEntry == 0)
        {
                similarInterest = false;
                pitEntry = m_pit->Create (interest);
                if (pitEntry != 0)
                {
                        DidCreatePitEntry (inFace, interest, pitEntry);
                }
                else
                {
                        FailedToCreatePitEntry (inFace, interest);
                        return;
                }
        }

        bool isDuplicated = true;
        if (!pitEntry->IsNonceSeen (interest->GetNonce ()))
        {
                pitEntry->AddSeenNonce (interest->GetNonce ());
                isDuplicated = false;
        }
        if (isDuplicated)
        {
                DidReceiveDuplicateInterest (inFace, interest, pitEntry);
                return;
        }

        Ptr<Data> contentObject;
        contentObject = m_contentStore->Lookup (interest);
        if (contentObject != 0)
        {
                FwHopCountTag hopCountTag;
                if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
                {
                contentObject->GetPayload ()->AddPacketTag (hopCountTag);
                }

                pitEntry->AddIncoming (inFace/*, Seconds (1.0)*/);

                // Do data plane performance measurements
                WillSatisfyPendingInterest (0, pitEntry);

                // Actually satisfy pending interest
                SatisfyPendingInterest (0, contentObject, pitEntry);
                return;
        }

        if (similarInterest && ShouldSuppressIncomingInterest (inFace, interest, pitEntry))
        {
                pitEntry->AddIncoming (inFace/*, interest->GetInterestLifetime ()*/);
                // update PIT entry lifetime
                pitEntry->UpdateLifetime (interest->GetInterestLifetime ());

                // Suppress this interest if we're still expecting data from some other face
                NS_LOG_DEBUG ("Suppress interests");
                m_dropInterests (interest, inFace);

                DidSuppressSimilarInterest (inFace, interest, pitEntry);
                return;
        }

        if (similarInterest)
        {
                DidForwardSimilarInterest (inFace, interest, pitEntry);
        }

        PropagateInterest (inFace, interest, pitEntry);
} //OnInterest close//

void
ForwardingStrategy::OnData (Ptr<Face> inFace,
                            Ptr<Data> data)
{
        NS_LOG_FUNCTION (inFace << data->GetName ());
        m_inData (data, inFace);

/////////////////////////////
/*  Ptr<Node> node = inFace -> GetNode();
  uint32_t nodeID = node -> GetId();
  Ptr<Limits> faceLimits = inFace -> GetObject<Limits>();
  double rate = faceLimits -> GetCurrentLimit();
  
  std::cout << Simulator::Now ().ToDouble (Time::S) << "\t" //time
            << nodeID << "\t"
            //<< inFace -> GetId() << "\t"  // incomingDataFaceID
            //<< m_pit->GetSize() << "\t"
            << rate << "\t"
            << "comeData" << "\n";
    */
///////////////////////////////  

        // Lookup PIT entry
        Ptr<pit::Entry> pitEntry = m_pit->Lookup (*data);
        if (pitEntry == 0)
        {
                bool cached = false;

                if (m_cacheUnsolicitedData || (m_cacheUnsolicitedDataFromApps && (inFace->GetFlags () & Face::APPLICATION)))
                {
                        // Optimistically add or update entry in the content store
                        cached = m_contentStore->Add (data);
                }
                else
                {
                        // Drop data packet if PIT entry is not found
                        // (unsolicited data packets should not "poison" content store)

                        //drop dulicated or not requested data packet
                        m_dropData (data, inFace);
                }
      
                DidReceiveUnsolicitedData (inFace, data, cached);
                return;
        }
        else
        {
                bool cached = m_contentStore->Add (data);
                DidReceiveSolicitedData (inFace, data, cached);
        }

        while (pitEntry != 0)
        {
                // Do data plane performance measurements
                WillSatisfyPendingInterest (inFace, pitEntry);

                // Actually satisfy pending interest

                FwFeedbackPitsizeTag feedbackPitsizeTag;
                data->GetPayload ()->PeekPacketTag(feedbackPitsizeTag);
 

                if (feedbackPitsizeTag.GetPitSize() >= 0)
                {      
      	                SatisfyPendingInterestDTCC (inFace, data, pitEntry);
                }
                else
                {
                        SatisfyPendingInterest (inFace, data, pitEntry);
                        //std::cout << "data包的Dh已经是0了，不用调整步数"<< "\n"; 
                }

                // Lookup another PIT entry
                pitEntry = m_pit->Lookup (*data);
        }
} //OnData close

void
ForwardingStrategy::DidCreatePitEntry (Ptr<Face> inFace,
                                       Ptr<const Interest> interest,
                                       Ptr<pit::Entry> pitEntrypitEntry)
{
}

void
ForwardingStrategy::FailedToCreatePitEntry (Ptr<Face> inFace,
                                            Ptr<const Interest> interest)
{
  m_dropInterests (interest, inFace);
}

void
ForwardingStrategy::DidReceiveDuplicateInterest (Ptr<Face> inFace,
                                                 Ptr<const Interest> interest,
                                                 Ptr<pit::Entry> pitEntry)
{
  /////////////////////////////////////////////////////////////////////////////////////////
  //                                                                                     //
  // !!!! IMPORTANT CHANGE !!!! Duplicate interests will create incoming face entry !!!! //
  //                                                                                     //
  /////////////////////////////////////////////////////////////////////////////////////////
  pitEntry->AddIncoming (inFace);
  m_dropInterests (interest, inFace);
}

void
ForwardingStrategy::DidSuppressSimilarInterest (Ptr<Face> face,
                                                Ptr<const Interest> interest,
                                                Ptr<pit::Entry> pitEntry)
{
}

void
ForwardingStrategy::DidForwardSimilarInterest (Ptr<Face> inFace,
                                               Ptr<const Interest> interest,
                                               Ptr<pit::Entry> pitEntry)
{
}

void
ForwardingStrategy::DidExhaustForwardingOptions (Ptr<Face> inFace,
                                                 Ptr<const Interest> interest,
                                                 Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << boost::cref (*inFace));
  if (pitEntry->AreAllOutgoingInVain ())
    {
      m_dropInterests (interest, inFace);

      // All incoming interests cannot be satisfied. Remove them
      pitEntry->ClearIncoming ();

      // Remove also outgoing
      pitEntry->ClearOutgoing ();

      // Set pruning timout on PIT entry (instead of deleting the record)
      m_pit->MarkErased (pitEntry);
    }
}



bool
ForwardingStrategy::DetectRetransmittedInterest (Ptr<Face> inFace,
                                                 Ptr<const Interest> interest,
                                                 Ptr<pit::Entry> pitEntry)
{
  pit::Entry::in_iterator existingInFace = pitEntry->GetIncoming ().find (inFace);

  bool isRetransmitted = false;

  if (existingInFace != pitEntry->GetIncoming ().end ())
    {
      // this is almost definitely a retransmission. But should we trust the user on that?
      isRetransmitted = true;
    }

  return isRetransmitted;
}

void
ForwardingStrategy::SatisfyPendingInterest (Ptr<Face> inFace,
                                            Ptr<const Data> data,
                                            Ptr<pit::Entry> pitEntry)
{
        if (inFace != 0)
                pitEntry->RemoveIncoming (inFace);

                //satisfy all pending incoming Interests
                BOOST_FOREACH (const pit::IncomingFace &incoming, pitEntry->GetIncoming ())
        {
                bool ok = incoming.m_face->SendData (data);

                DidSendOutData (inFace, incoming.m_face, data, pitEntry);
                NS_LOG_DEBUG ("Satisfy " << *incoming.m_face);

                if (!ok)
                {
                        m_dropData (data, incoming.m_face);
                        NS_LOG_DEBUG ("Cannot satisfy data to " << *incoming.m_face);
                }
        }

        // All incoming interests are satisfied. Remove them
        pitEntry->ClearIncoming ();

        // Remove all outgoing faces
        pitEntry->ClearOutgoing ();

        // Set pruning timout on PIT entry (instead of deleting the record)
        m_pit->MarkErased (pitEntry);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void
ForwardingStrategy::SatisfyPendingInterestDTCC (Ptr<Face> inFace,
                                            Ptr<Data> data,
                                            Ptr<pit::Entry> pitEntry)
{
        

        if (inFace != 0)
                pitEntry->RemoveIncoming (inFace);
        	//std::cout << Simulator::Now ().ToDouble (Time::S) << "\n"; // time

  	        //satisfy all pending incoming Interests
  	        BOOST_FOREACH (const pit::IncomingFace &incoming, pitEntry->GetIncoming ())
  	{

   		///2018/1/4/////////making feedback////////////////
		//const int max_number_of_face = 4; //ノードのとりうるFaceの最大数  
                const int max_number_of_face = 10; //5src.cc by aikawa 

                uint32_t outFace_data = incoming.m_face -> GetId(); // このデータが出て行くfaceID 

                //std::cout << "this Data will go to Face No." << outFace_data << "\n";
     	        uint32_t inPitsize=0; //このデータが出て行くFaceのピットのサイズ。重み付けの分母。
     	        uint32_t inPitsize_face[max_number_of_face] = {}; //このデータに関連する出て行くピットサイズ。重み付けの分子。

     	        bool ExistOutFace_data=false; //このデータが出て行くFaceに関連するPITエントリかどうかを調べるフラグ。

     	        double outPitsize[max_number_of_face] = {}; //出て行くインタレストに関するピットのサイズ。ピットサイズ差算出に使う。各FaceのPITサイズ。

     	        double outPitsizedif[max_number_of_face] = {}; //出て行くinterstに関するピット(outPIT)サイズの差。重み付けされる。各Faceの待機パケット数。
     	        double interestRate[max_number_of_face] = {}; //このデータに関するPITに関するInterest送出Faceの送出レート。重み付けされる。各Faceの送出レート。

    	        Ptr<pit::Entry> pitEntry2 = m_pit -> Begin(); //PITエントリの最初
      	        //int count=0;

    	        while(pitEntry2) //PITエントリを全部見る
	  	{
			//std::cout << "incomingFace is " << "\t";

			std::set<ndn::pit::IncomingFace> incoming_face  = pitEntry2 -> GetIncoming(); //incomingFaceをみる（インタレストが入ってきたFace）
			for(std::set<ndn::pit::IncomingFace>::iterator in_itr = incoming_face.begin(); in_itr != incoming_face.end(); ++in_itr)
   		        {
     		                uint32_t infaceId;
				infaceId = in_itr -> m_face -> GetId();
				//std::cout << infaceId << ",";
				if(infaceId == outFace_data) // このデータが出て行くFaceに関するPITサイズの計算
				{
					inPitsize++;
				}
			}

                        if(inPitsize>0) ExistOutFace_data = true;

			//std::cout << "\n";

			//std::cout << "outgoingFace is " << "\t";

			std::set<ndn::pit::OutgoingFace> outgoing_face  = pitEntry2 -> GetOutgoing(); //outgoingFaceをみる（インタレストが出て行ったFace）
   		        for(std::set<ndn::pit::OutgoingFace>::iterator out_itr = outgoing_face.begin(); out_itr != outgoing_face.end(); ++out_itr)
   		        {
     		                uint32_t outfaceId;
				outfaceId = out_itr -> m_face -> GetId();
				outPitsize[outfaceId] = outPitsize[outfaceId] + 1; //Face毎の出て行ったインタレストに関するPIT（OutPITと呼ぶ）のサイズを計算

				outPitsizedif[outfaceId] = out_itr -> m_face -> GetPitsizedif(); //face毎のPITサイズ差
				interestRate[outfaceId] = out_itr -> m_face -> GetObject<Limits>() -> GetCurrentLimit(); //Face毎の転送レート

				if(ExistOutFace_data) //今回のデータが出て行くFaceに関するフィードバック情報を計算する
			        {		
					inPitsize_face[outfaceId] = inPitsize_face[outfaceId] + 1;
				}
			        //std::cout << outfaceId << ",";
   		        }
    		        //std::cout << "\n";

    	                pitEntry2 = m_pit -> Next(pitEntry2);
	  		//count++; // count pit size
   		        ExistOutFace_data =false;
  	        }  //while() close//

                /*
	  	if(inFace -> GetNode() -> GetId() == 8 || inFace -> GetNode() -> GetId() == 10)
                {
       		        std::cout << Simulator::Now ().ToDouble (Time::S) << "\t"; // time
       		        std::cout << inFace -> GetNode() -> GetId() << "\t"; // nodeID
       		        std::cout << Names::FindName(inFace -> GetNode() ) << "\t";// nodeName
	      	        std::cout << "inPitsize is " << inPitsize << "\n";
		  	for(int i=0 ; i<max_number_of_face ;i++)
                        {
	  		        std::cout << Simulator::Now ().ToDouble (Time::S) << "\t"
                                          << Names::FindName(inFace -> GetNode() ) << "\t"
                                          << i << "\t"
                                          << inPitsize_face[i] << "\t"
	  				  << outPitsizedif[i] << "\t"
	  				  << interestRate[i] << "\t"
	  			  	  << outPitsize[i] << "\n";
	  		}
	  	}*/
	  	
      	        /*if(inFace -> GetNode() -> GetId() == 8 || inFace -> GetNode() -> GetId() == 10)
                {
       		        std::cout << Simulator::Now ().ToDouble (Time::S) << "\t"; // time
       		        std::cout << inFace -> GetNode() -> GetId() << "\t"; // nodeID
       		        std::cout << Names::FindName(inFace -> GetNode() ) << "\t";// nodeName
	      	        std::cout << "inPitsize is " << inPitsize << "\n";
		  	for(int i=0 ; i<max_number_of_face ;i++)
                        {
	  		        std::cout << Simulator::Now ().ToDouble (Time::S) << "\t"
                                          << Names::FindName(inFace -> GetNode() ) << "\t"
                                          << "inPitsize_face[" << i << "] is " << inPitsize_face[i] << "\t"
	  				  << "outPitsizedif[" << i << "] is " << outPitsizedif[i] << "\t"
	  				  << "interestRate[" << i << "] is " << interestRate[i] << "\t"
	  			  	  << "outPitsize[" << i << "] is " << outPitsize[i] << "\n";
	  		}
	  	}*/
	  	

	  	// std::cout << "PIT SIZE = " << count << "\n";

	  	//2018/1/4///////////////////////////////////////////////////////////////////////////////////////////

      	        //2017/12/14///////////////////////////////////////////////////////////////////////////////////////////
      

      	        Ptr<Node> node = inFace -> GetNode();
      	        uint32_t nodeID = node -> GetId();
      	        uint32_t infaceId = inFace -> GetId(); //データが入ってきたFaceのID

                DataPacketNum[nodeID]++;//Lee

              	/* create a new FeedbackTag */
              	FwFeedbackPitsizeTag feedbackPitsizeTag;
              	FwFeedbackRateTag feedbackRateTag;
              	FwFeedbackPitsizeDifferenceTag feedbackPitsizeDifferenceTag;        

              	data -> GetPayload() ->  PeekPacketTag(feedbackPitsizeTag);
              	data -> GetPayload() ->  PeekPacketTag(feedbackRateTag);
              	data -> GetPayload() ->  PeekPacketTag(feedbackPitsizeDifferenceTag);

              	double f_pitsize = feedbackPitsizeTag.GetPitSize();
              	double f_rate = feedbackRateTag.GetRate();
        	double f_pitsizedif = feedbackPitsizeDifferenceTag.GetPitSizeDif();
                /*
                if(nodeID == 8 || nodeID == 10){
	  	std::cout 
	  	<< "feedbackpitsize: " << f_pitsize << "\t"
	  	<< "feedbackrate: " <<f_rate << "\t"
	  	<< "feedbackpitsizedif: " << f_pitsizedif <<"\n";
		}*/

	  	if(f_pitsizedif != 0 || f_pitsize != 0 || f_rate != 0)
	  	{
	  	        //check whether tag set or no!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!///////
	   	        //data tags maid by intermadiate router doesn't have any value./////////  

	      	        /* get pitsize */
    		        double pitsize = outPitsize[infaceId]; // データが入ってきたFaceのPITサイズ ///LEE: InPIT
   		        double pitsizedif = pitsize - f_pitsize; // データが入ってきたFaceのPITサイズ差  ///LEE : InPIT-OutPIT
        if (nodeID == 14){
          NS_LOG_DEBUG ("nodeID 14: pitsize: " << pitsize );
        }
	                //if(nodeID == 3 || nodeID == 4) //for ndn-congestion-topo-plugin-star.cc
           	        //if(nodeID == 17) //for ndn-congestion-topo-plugin-test.cc
                        //if(nodeID == 19  || nodeID == 20) ////201021 for ndn-congestion-topo-plugin-test-2src.cc
                        //if(nodeID == 4 || nodeID == 5)
                        //if(nodeID == 10 || nodeID == 9) //200210705 for ndn-congestion-topo-ring-2src.cc
                        //if(nodeID == 10 || nodeID == 11 || nodeID == 12) //200210706 for ndn-congestion-topo-plugin-3src.cc
                        //if(nodeID == 8 || nodeID == 9 || nodeID == 10 || nodeID == 11) //200210706 for ndn-congestion-topo-4src.cc
                        //if(nodeID == 15 || nodeID == 16 || nodeID == 17 || nodeID == 18) //200210706 for ndn-congestion-topo-4src.cc
                        //if(nodeID == 8 || nodeID == 9 || nodeID == 10) //20211012 for ndn-congestion-topo-tree-3src.cc
                        if(nodeID == 16 || nodeID == 17 || nodeID == 18 || nodeID == 19 || nodeID == 20) //20211019 for ndn-congestion-topo54src.cc
                        {
    	    	                pitsize = 0;
        		        pitsizedif = 0; // Half size of uint32_t (MAX uint32_t = 4294967295 )
                        } 

         
	    	        /* get current interest sending rate limit */
      		        Ptr<Limits> faceLimits = inFace -> GetObject<Limits>();
      		        double rate = faceLimits -> GetCurrentLimit();
      		        //std::cout << "cur_rate = " << rate << "\n";
        
           
                        //if(nodeID == 3 || nodeID== 4)  //for ndn-congestion-topo-plugin-star.cc 
                        //if(nodeID == 17 ) ////for ndn-congestion-topo-plugin-test.cc
                        //if(nodeID == 19  || nodeID == 20) ////201021 for ndn-congestion-topo-plugin-test-2src.cc
                        //if(nodeID == 4 || nodeID == 5)
                        //if(nodeID == 10 || nodeID == 9) //200210705 for ndn-congestion-topo-ring-2src.cc
                        //if(nodeID == 10 || nodeID == 11 || nodeID == 12) //200210706 for ndn-congestion-topo-plugin-3src.cc
                        //if(nodeID == 8 || nodeID == 9 || nodeID == 10 || nodeID == 11) //200210706 for ndn-congestion-topo-4src.cc
                        //if(nodeID == 15 || nodeID == 16 || nodeID == 17 || nodeID == 18) //200210706 for ndn-congestion-topo-4src.cc
                        //if(nodeID == 8 || nodeID == 9 || nodeID == 10) //20211012 for ndn-congestion-topo-tree-3src.cc
                        if(nodeID == 16 || nodeID == 17 || nodeID == 18 || nodeID == 19 || nodeID == 20) //20211019 for ndn-congestion-topo54src.cc
                        {
      			        rate = 100000;
     		        }  
                         
                
                                 
      		        /* update interest sendinrg rate limit */
        	        //if(nodeID != 3 && nodeID != 4) //for ndn-congestion-topo-plugin-star.cc 
      		        //if(nodeID != 17 ) ////for ndn-congestion-topo-plugin-test.cc
                        //if(nodeID != 19  && nodeID != 20) ////201021 for ndn-congestion-topo-plugin-test-2src.cc
                        //if(nodeID != 4 && nodeID != 5)
                        //if(nodeID != 10 && nodeID != 9) //200210705 for ndn-congestion-topo-ring-2src.cc
                        //if(nodeID != 10 && nodeID != 11 && nodeID != 12) //200210706 for ndn-congestion-topo-plugin-3src.cc
                        //if(nodeID != 8 && nodeID != 9 && nodeID != 10 && nodeID != 11) //200210706 for ndn-congestion-topo-4src.cc
                        //if(nodeID != 15 && nodeID != 16 && nodeID != 17 && nodeID != 18) //200210706 for ndn-congestion-topo-4src.cc
                        //if(nodeID != 8 && nodeID != 9 && nodeID != 10) //20211012 for ndn-congestion-topo-tree-3src.cc
                        if(nodeID != 16 && nodeID != 17 && nodeID != 18 && nodeID != 19 && nodeID != 20) //20211019 for ndn-congestion-topo54src.cc
                        {
                                //if(nodeID == 0 )//for src 1 
                                //if(nodeID == 0 || nodeID == 1)
                                //if(nodeID == 0 || nodeID == 1 || nodeID == 2)//for src 1 , src 2, src3 //aikawa
                                //if(nodeID == 0 || nodeID == 1 || nodeID == 2 || nodeID == 3)//for src 1 , src 2, src3, src4 //aikawa
                                if(nodeID == 0 || nodeID == 1 || nodeID == 2 || nodeID == 3 || nodeID ==4)//for src 1 , src 2, src3, src4, src5//aikawa
                                {
                                        rate = 100000;
                                }//lee
                               
                           //     if(infaceId == 2){
                           //             pitsize = outPitsize[0] + outPitsize[1];
                           //             pitsizedif = pitsize - 2*f_pitsize;
                           //    }
                                //lee //only Rtr9 has infaceId 2 in topology 2
                           //     if(infaceId == 7){
                           //             pitsize =  outPitsize[3] + outPitsize[4];
                           //             pitsizedif = pitsize - 2*f_pitsize;
                           //     }//aikawa
                        
                                
        		        double d = 0.1;                          
                    double oldRate = rate;
                                //rate = f_rate - d*(f_pitsizedif - pitsizedif); //(origin) rate計算の式        		
                                rate = f_rate - d*(f_pitsizedif - pitsizedif + DataPacketNum[nodeID]); //LEE's proposal                    

                    if((nodeID == 10 && infaceId == 7 ) || nodeID == 8 || nodeID == 14){
                      NS_LOG_DEBUG("Node " << nodeID << " infaceID " << infaceId << ": old rate:" << oldRate
                      << " new rate:" << rate << " f_rate:" << f_rate);
                      std::cout << "---------------------------" << std::endl;
                    }
         
///2019/1/////Harada Masahiro//////////////////////////////////////////////////////////////////
//              
//              
//                              if( (pitsizedif >= 0 && pitsizedif<5) ){
//                                      ad = 10; 
//                                      ad = pitsizedif ; 
//                                  
//                                      rate= rate+ad; 
//                                      ad = ad+0.1 ;                                       
//                                      rate = rate + pow((5-ad),2) ;
//                        
//                         
//                              }
//                              if(pitsizedif >= 5){
//                                      rate = rate/2;
//                                      ad = ad/2;
//                                      if(ad<5.0){
//                                              ad = 5.0;
//                                      }
//                              
//                              }
////End 2019/1////////////////////////////////////////////////////////////////////////
                                //if(nodeID == 8 || nodeID == 10){
                                //        std::cout << "nowLimit : "<< faceLimits -> GetCurrentLimit() << "\n";
                                //}


	   		        if(rate < 0) rate = 1;
		                faceLimits -> UpdateCurrentLimit(rate);
        		        rate = faceLimits -> GetCurrentLimit();
                    if((nodeID == 10 && infaceId == 7) || nodeID == 8 || nodeID == 14){
                      NS_LOG_DEBUG("Node " << nodeID << " infaceID " << infaceId << " | rate:" << rate
                        << " pitsizedif:" << pitsizedif << " f_pitsizedif:" << f_pitsizedif
                        << " DataPacketNum:" << DataPacketNum[nodeID] 
                        << " face-info:" << *inFace);
                      std::cout << "---------------------------" << std::endl;
                    }
                  /*              if(nodeID == 2 || nodeID == 3 || nodeID == 7 || nodeID == 8 || nodeID ==10 || nodeID == 14 || nodeID == 15 || nodeID == 18 || nodeID == 19){
      			                std::cout << Simulator::Now ().ToDouble (Time::S) << "\t"; // time
      	 		                std::cout << inFace -> GetNode() -> GetId() << "\t"; // nodeID
       			                std::cout << Names::FindName(inFace -> GetNode() ) << "\t"; // nodeName
        		
        		                std::cout << pitsize << "\t" <<  d << "\t"; 
        		                std::cout << f_pitsize << "\t" << f_rate  << "\t" << f_pitsizedif << "\t"
 		                                  << pitsizedif << "\t" 	
        		                          << rate << "\t";
        		                
                                        std::cout << inFace->GetId() << "\t";
        		                std::cout << outFace_data << "\t";
                                        std::cout << DataPacketNum[nodeID] << "\n";
                                        //std::cout << "newLimit : "<< faceLimits -> GetCurrentLimit() << "\n";
        		        }*/
                                /*
                                if(nodeID == 8 || nodeID ==10){
      			                std::cout << "\n" << Simulator::Now ().ToDouble (Time::S) << "\t"; // time
      	 		                std::cout << inFace -> GetNode() -> GetId() << "\t"; // nodeID
       			                std::cout << Names::FindName(inFace -> GetNode() ) << "\n"; // nodeName
        		
        		                std::cout << " pitsize: " << pitsize << " d:" << d << "\n"; 
        		                std::cout << " f_pitsize:" << f_pitsize << " f_rate:" << f_rate  << " f_pitsizedif:" << f_pitsizedif << "\n"
 		                                  << " pitsizedif:" << pitsizedif << "\n" 	
            		                          << " rate = f_rate - d*(f_pitsizedif - pitsizedif + DataPacketNum[nodeID]) is" << "\n"
        		                          << rate << " = " << f_rate << " - " << d << "*( " << f_pitsizedif << " - " << pitsizedif << "+" << DataPacketNum[nodeID] << " )" << "\n";
        		                std::cout << "m_bucketLeak = " << rate << "\n";
                                        std::cout << "DATA_send_from_faceID_=_" << inFace->GetId() << "\n";
        		                std::cout << "DATA_send_to_faceID_=_" << outFace_data << "\n";
        		        }
                                */
        		        //std::cout << "!!!!!!!!!!__pitsizedif_didn't set_the value_at_neighbor_router__!!!!!!!!!!" << "\n";
        

       			        outPitsizedif[infaceId] = pitsizedif; //データの更新
       			        inFace->SetPitsizedif(pitsizedif);
       			        interestRate[infaceId] = rate;

        		        double f_pitsize_test=0;
    	    	                double f_pitsizedif_test=0;
    	    	                double f_rate_test=0;
               

	    	                f_pitsize_test = inPitsize;
	    	                //std::cout << "f_pitsize_test is " << f_pitsize_test << "\n";
	    	                pitsize= f_pitsize_test;

	    	                //std::cout << "f_pitsize_test is " << f_pitsize_test << " and pitsize is " << pitsize << "\n"; 
	    	                double w[max_number_of_face] ={};

	    	                for(int i=0;i<max_number_of_face;i++)
                                {
    				        w[i] = (double)inPitsize_face[i]/(double)inPitsize;
    			                //std::cout << "w[" << i << "] is " << w[i] << "\n";
    		                }

    	   		        double w_f_pitsizedif[max_number_of_face]={};
   		 		for(int i=0;i<max_number_of_face;i++)
                                {
    			   	        w_f_pitsizedif[i] = w[i]*outPitsizedif[i];
    			   	        //std::cout << "w_f_pitsizedif[" << i << "] is " << w_f_pitsizedif[i] << "\n";
    			   	        f_pitsizedif_test = f_pitsizedif_test + w_f_pitsizedif[i];
    			        }
    	  		        //std::cout << "f_pitsizedif_test is " << f_pitsizedif_test << "\n";       

    	  		        //pitsizedif = f_pitsizedif_test;

    	  	        	double w_f_rate[max_number_of_face]={};
    	   		        for(int i=0;i<max_number_of_face;i++)
                                {
    	   		        	w_f_rate[i] = w[i] * interestRate[i];
    	   		                //std::cout << "w_f_rate[" << i << "] is " << w_f_rate[i] << "\n";
    	   			        f_rate_test = f_rate_test + w_f_rate[i];
    	   		        }
    	   		        //std::cout << "f_rate_test is " << f_rate_test << "\n";
            	   		//rate = f_rate_test;                        
                                //2019/1/switch dfcc or non_dfcc ??

              		} //if(nodeID != 19  && nodeID != 20) close
                        		




      		        //std::cout << " upd_rate = " << rate << "\n"; 
 
		        /* remove old tag? */      		
    		        Ptr<Packet> payloadCopy = data -> GetPayload() -> Copy();
    		        bool pitsizetagExist = payloadCopy -> RemovePacketTag(feedbackPitsizeTag);
      		        bool ratetagExist = payloadCopy -> RemovePacketTag(feedbackRateTag);
      		        bool pitsizediftagExist = payloadCopy -> RemovePacketTag(feedbackPitsizeDifferenceTag);

      		        if(pitsizetagExist && ratetagExist && pitsizediftagExist)
      		        { 
      		                /* set pitsize on feedback */
      			        feedbackPitsizeTag.SetPitSize(pitsize);
      			        payloadCopy -> AddPacketTag(feedbackPitsizeTag);
      			        //std::cout << "feedbackPitsizeTag.GetPitSize() = " << feedbackPitsizeTag.GetPitSize() << "\n";
      			        data->SetPayload (payloadCopy);
      

     			        /* set rate on feedback */
      			        feedbackRateTag.SetRate(rate);
      			        payloadCopy -> AddPacketTag(feedbackRateTag);
      			        //std::cout << "feedbackRateTag.GetRate() = " << feedbackRateTag.GetRate() << "\n";
      			        data->SetPayload (payloadCopy);

        
      			        feedbackPitsizeDifferenceTag.SetPitSizeDif(pitsizedif);
      			        payloadCopy -> AddPacketTag(feedbackPitsizeDifferenceTag);
      			        //std::cout << "feedbackPitsizeDifferenceTag.GetPitDifSize() = " << feedbackPitsizeDifferenceTag.GetPitDifSize() << "\n";
      			        data->SetPayload (payloadCopy);
      		        }
     	        } //if(f_pitsizedif != 0 || f_pitsize != 0 || f_rate != 0) close
                //2017/12/14////////////////////////////////////////////////////////////////////////////////////////////////////////////////




      	        bool ok = incoming.m_face->SendData (data);

      	        DidSendOutData (inFace, incoming.m_face, data, pitEntry);
                DataPacketNum[nodeID]--;
      	        NS_LOG_DEBUG ("Satisfy " << *incoming.m_face);

      	        if (!ok)
                {
          	        m_dropData (data, incoming.m_face);
          	        NS_LOG_DEBUG ("Cannot satisfy data to " << *incoming.m_face);
                }
   
        } //if (inFace != 0) close

        // All incoming interests are satisfied. Remove them
        pitEntry->ClearIncoming ();

        // Remove all outgoing faces
        pitEntry->ClearOutgoing ();

        // Set pruning timout on PIT entry (instead of deleting the record)
        m_pit->MarkErased (pitEntry);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
ForwardingStrategy::DidReceiveSolicitedData (Ptr<Face> inFace,
                                             Ptr<const Data> data,
                                             bool didCreateCacheEntry)
{
  // do nothing
}

void
ForwardingStrategy::DidReceiveUnsolicitedData (Ptr<Face> inFace,
                                               Ptr<const Data> data,
                                               bool didCreateCacheEntry)
{
  // do nothing
}

void
ForwardingStrategy::WillSatisfyPendingInterest (Ptr<Face> inFace,
                                                Ptr<pit::Entry> pitEntry)
{
  pit::Entry::out_iterator out = pitEntry->GetOutgoing ().find (inFace);

  // If we have sent interest for this data via this face, then update stats.
  if (out != pitEntry->GetOutgoing ().end ())
    {
      pitEntry->GetFibEntry ()->UpdateFaceRtt (inFace, Simulator::Now () - out->m_sendTime);
    }

  m_satisfiedInterests (pitEntry);
}

bool
ForwardingStrategy::ShouldSuppressIncomingInterest (Ptr<Face> inFace,
                                                    Ptr<const Interest> interest,
                                                    Ptr<pit::Entry> pitEntry)
{
  bool isNew = pitEntry->GetIncoming ().size () == 0 && pitEntry->GetOutgoing ().size () == 0;

  if (isNew) return false; // never suppress new interests

  bool isRetransmitted = m_detectRetransmissions && // a small guard
                         DetectRetransmittedInterest (inFace, interest, pitEntry);

  if (pitEntry->GetOutgoing ().find (inFace) != pitEntry->GetOutgoing ().end ())
    {
      NS_LOG_DEBUG ("Non duplicate interests from the face we have sent interest to. Don't suppress");
      // got a non-duplicate interest from the face we have sent interest to
      // Probably, there is no point in waiting data from that face... Not sure yet

      // If we're expecting data from the interface we got the interest from ("producer" asks us for "his own" data)
      // Mark interface YELLOW, but keep a small hope that data will come eventually.

      // ?? not sure if we need to do that ?? ...

      // pitEntry->GetFibEntry ()->UpdateStatus (inFace, fib::FaceMetric::NDN_FIB_YELLOW);
    }
  else
    if (!isNew && !isRetransmitted)
      {
        return true;
      }

  return false;
}

void
ForwardingStrategy::PropagateInterest (Ptr<Face> inFace,
                                       Ptr<const Interest> interest,
                                       Ptr<pit::Entry> pitEntry)
{
  bool isRetransmitted = m_detectRetransmissions && // a small guard
                         DetectRetransmittedInterest (inFace, interest, pitEntry);

  pitEntry->AddIncoming (inFace/*, interest->GetInterestLifetime ()*/);
  /// @todo Make lifetime per incoming interface
  pitEntry->UpdateLifetime (interest->GetInterestLifetime ());

  bool propagated = DoPropagateInterest (inFace, interest, pitEntry);

  if (!propagated && isRetransmitted) //give another chance if retransmitted
    {
      // increase max number of allowed retransmissions
      pitEntry->IncreaseAllowedRetxCount ();
      
      // try again
      propagated = DoPropagateInterest (inFace, interest, pitEntry);
    }

  // if (!propagated)
  //   {
  //     NS_LOG_DEBUG ("++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  //     NS_LOG_DEBUG ("+++ Not propagated ["<< interest->GetName () <<"], but number of outgoing faces: " << pitEntry->GetOutgoing ().size ());
  //     NS_LOG_DEBUG ("++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  //   }

  // ForwardingStrategy will try its best to forward packet to at least one interface.
  // If no interests was propagated, then there is not other option for forwarding or
  // ForwardingStrategy failed to find it.
  if (!propagated && pitEntry->AreAllOutgoingInVain ())
    {
      DidExhaustForwardingOptions (inFace, interest, pitEntry);
    }
}

bool
ForwardingStrategy::CanSendOutInterest (Ptr<Face> inFace,
                                        Ptr<Face> outFace,
                                        Ptr<const Interest> interest,
                                        Ptr<pit::Entry> pitEntry)
{
  if (outFace == inFace)
    {
      // NS_LOG_DEBUG ("Same as incoming");
      return false; // same face as incoming, don't forward
    }

  pit::Entry::out_iterator outgoing =
    pitEntry->GetOutgoing ().find (outFace);
NS_LOG_DEBUG(pitEntry->GetOutgoing().size());
  if (outgoing != pitEntry->GetOutgoing ().end ())
    {
      
      if (!m_detectRetransmissions)
        return false; // suppress
      else if (outgoing->m_retxCount >= pitEntry->GetMaxRetxCount ())
        {
          // NS_LOG_DEBUG ("Already forwarded before during this retransmission cycle (" <<outgoing->m_retxCount << " >= " << pitEntry->GetMaxRetxCount () << ")");
          return false; // already forwarded before during this retransmission cycle
        }
   }

  return true;
}


bool
ForwardingStrategy::TrySendOutInterest (Ptr<Face> inFace,
                                        Ptr<Face> outFace,
                                        Ptr<const Interest> interest,
                                        Ptr<pit::Entry> pitEntry)
{
  if (!CanSendOutInterest (inFace, outFace, interest, pitEntry))
    {
      return false;
    }

  pitEntry->AddOutgoing (outFace);

  //transmission
  bool successSend = outFace->SendInterest (interest);
  if (!successSend)
    {
      m_dropInterests (interest, outFace);
    }

  DidSendOutInterest (inFace, outFace, interest, pitEntry);

  return true;
}

void
ForwardingStrategy::DidSendOutInterest (Ptr<Face> inFace,
                                        Ptr<Face> outFace,
                                        Ptr<const Interest> interest,
                                        Ptr<pit::Entry> pitEntry)
{
  m_outInterests (interest, outFace);
}

void
ForwardingStrategy::DidSendOutData (Ptr<Face> inFace,
                                    Ptr<Face> outFace,
                                    Ptr<const Data> data,
                                    Ptr<pit::Entry> pitEntry)
{
  m_outData (data, inFace == 0, outFace);
  
}

void
ForwardingStrategy::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
  m_timedOutInterests (pitEntry);
}

void
ForwardingStrategy::AddFace (Ptr<Face> face)
{
  // do nothing here
}

void
ForwardingStrategy::RemoveFace (Ptr<Face> face)
{
  // do nothing here
}

void
ForwardingStrategy::DidAddFibEntry (Ptr<fib::Entry> fibEntry)
{
  // do nothing here
}

void
ForwardingStrategy::WillRemoveFibEntry (Ptr<fib::Entry> fibEntry)
{
  // do nothing here
}


} // namespace ndn
} // namespace ns3
