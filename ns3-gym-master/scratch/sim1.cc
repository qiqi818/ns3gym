/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Piotr Gawlowicz
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
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#include "ns3/core-module.h"
#include "ns3/opengym-module.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"


#include <ns3/object.h>
#include <ns3/spectrum-interference.h>
#include <ns3/spectrum-error-model.h>
#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/ptr.h>

#include "ns3/radio-bearer-stats-calculator.h"
#include <ns3/constant-position-mobility-model.h>
#include <ns3/eps-bearer.h>
#include <ns3/node-container.h>
#include <ns3/mobility-helper.h>
#include <ns3/net-device-container.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/lte-enb-net-device.h>
#include <ns3/lte-ue-rrc.h>
#include <ns3/lte-helper.h>
#include "ns3/string.h"
#include "ns3/double.h"
#include <ns3/lte-enb-phy.h>
#include <ns3/lte-ue-phy.h>
#include <ns3/boolean.h>
#include <ns3/enum.h>
#include <ns3/string.h>
#include <ns3/epc-helper.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include <ns3/object-factory.h>
//Add for input-defaults.txt file
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include<iostream>

//FlowMonitor
#include "ns3/flow-monitor-module.h"


using namespace ns3;
using namespace std;
Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
NetDeviceContainer enbDevs;
NetDeviceContainer ueDevs;
NS_LOG_COMPONENT_DEFINE ("OpenGym");

/*
Define observation space
*/
Ptr<OpenGymSpace> MyGetObservationSpace(void)
{
  cout << "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+" << endl;
  
  uint32_t nodeNum = 5;
  float low = 0.0;
  float high = 10.0;
  std::vector<uint32_t> shape = {nodeNum,};
  std::string dtype = TypeNameGet<uint32_t> ();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);

  return space;
}

/*
Define action space
*/
Ptr<OpenGymSpace> MyGetActionSpace(void)
{
  Ptr<OpenGymTupleSpace> space = CreateObject<OpenGymTupleSpace> ();
  for(int i = 0; i < 1000; i++)
    space->Add(CreateObject<OpenGymDiscreteSpace> (1));

  return space;
}

/*
Define game over condition
*/
bool MyGetGameOver(void)
{

  bool isGameOver = false;
  // bool test = false;
  static float stepCounter = 0.0;
  stepCounter += 1;
  // if (stepCounter == 10 && test) {
  //     isGameOver = true;
  // }
  // cout << "step: " << stepCounter << endl;
  NS_LOG_UNCOND ("MyGetGameOver: " << isGameOver);
  
  return isGameOver;
}

/*
Collect observations
*/
Ptr<OpenGymDataContainer> MyGetObservation(void)
{
  // ifstream file;
  // file.open("/home/liqi/ns3-gym/ns3-gym-master/DlMacStats.txt");
  // char szbuff[1024] = {0};
  // string za;
  // while(!file.eof()){
  //   file.getline(szbuff,1024);
  //   za = szbuff;
  //   cout << za << endl;
  // }
  //ueDevs.Get(2)->GetAttribute
  //ueDevs.Get(0)->GetNode()->
  
  cout << Simulator::Now() << endl;
  uint32_t nodeNum = 7;
  std::vector<uint32_t> shape = {nodeNum,};
  Ptr<OpenGymBoxContainer<uint32_t> > box = CreateObject<OpenGymBoxContainer<uint32_t> >(shape);


  for(uint16_t i = 0; i < 7; i++)
  {    
    PointerValue ptr;
    //enbDevs.Get(i)-> GetObject<LteUeNetDevice> ()->
    enbDevs.Get(i)-> GetObject<LteEnbNetDevice> ()->GetCcMap().at(0)->GetAttribute("FfMacScheduler",ptr);
    Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler>();
     Ptr<PfFfMacScheduler> pff = ff->GetObject<PfFfMacScheduler>();
     int count = 0;
     for(int m = 0;  m < 50; m++)
     {
       
       if(pff-> LcActivePerFlow(m)>0) 
          count++;
     }
     box->AddValue(count);   
  }

  cout<<"MyGetObservation: "<<box<<endl;
  return box;
}



/*
Define extra info. Optional
*/
std::string MyGetExtraInfo(void)
{
  std::string myInfo = "testInfo";
  myInfo += "|123";
  NS_LOG_UNCOND("MyGetExtraInfo: " << myInfo);
  return myInfo;
}


/*
Execute received actions
*/
bool MyExecuteActions(Ptr<OpenGymDataContainer> action)
{
  Ptr<OpenGymTupleContainer> tu_ac = DynamicCast<OpenGymTupleContainer>(action);
  
  string ac_s[7] = {"","","","","","",""};
  
  
  for(uint32_t i = 0 ; i < tu_ac->size(); i++){
    if(DynamicCast<OpenGymDiscreteContainer>(tu_ac->Get(i))->GetValue() == 9999)
    {
      ac_s[0] += to_string(9999);
      ac_s[1] += to_string(9999);
      ac_s[2] += to_string(9999);
      ac_s[3] += to_string(9999);
      ac_s[4] += to_string(9999);
      ac_s[5] += to_string(9999);
      ac_s[6] += to_string(9999);

    }
    else
    {
      ac_s[DynamicCast<OpenGymDiscreteContainer>(tu_ac->Get(i))->GetValue()/12] += to_string(DynamicCast<OpenGymDiscreteContainer>(tu_ac->Get(i))->GetValue()%12);
      ac_s[DynamicCast<OpenGymDiscreteContainer>(tu_ac->Get(i))->GetValue()/12] += " ";
    }
  
  }
  cout << endl; 
  cout << ac_s << endl; 
  
  
  
  NS_LOG_UNCOND ("MyExecuteActions: " << tu_ac);
for(uint16_t i = 0; i < 7; i++)
  {
   
    
    PointerValue ptr;
    enbDevs.Get(i)-> GetObject<LteEnbNetDevice> ()->GetCcMap().at(0)->GetAttribute("FfMacScheduler",ptr);
    Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler>();
   
   
    StringValue c;
    ff->SetAttribute("test",StringValue(ac_s[i]));
    ff->GetAttribute("test",c);
    cout <<"第"<<i<<"个波束分配第" <<c.Get() << "个RBG"<< endl;
  }
  cout << "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+" << endl;



  return true;
}


/*
Define reward function
*/
float MyGetReward(void)
{
  float reward = 0.0;
  reward = 1;
  return reward;
}

void ScheduleNextStateRead(double envStepTime, Ptr<OpenGymInterface> openGym)
{

  Simulator::Schedule (Seconds(envStepTime), &ScheduleNextStateRead, envStepTime, openGym);
  openGym->NotifyCurrentState();
}




double U_Random(uint16_t k)   /* 产生一个0~1之间的随机数 */
{
        double f;
        srand( k );
        f = (float)(rand() % 100);
        /* printf("%fn",f); */
        return f/100;
}

 
int possion(int lambda, double u)  /* 产生一个泊松分布的随机数，Lamda为总体平均数*/
{
        int Lambda = 10, k = 0;
         double p = 1.0;
         double l=exp(-Lambda);  /* 为了精度，才定义为long double的，exp(-Lambda)是接近0的小数*/
        while (p>=l)
        {
                
                p *= u;
                k++;
        }
        return k-1;
}



int
main (int argc, char *argv[])
{
  // Parameters of the scenario
  uint32_t simSeed = 1;
  double simulationTime = 5; //seconds
  double envStepTime = 0.001; //seconds, ns3gym env step time interval
  uint32_t openGymPort = 5555;
  uint32_t testArg = 0;
  CommandLine cmd;
  // required parameters for OpenGym interface
  cmd.AddValue ("openGymPort", "Port number for OpenGym env. Default: 5555", openGymPort);
  cmd.AddValue ("simSeed", "Seed for random generator. Default: 1", simSeed);
  // optional parameters
  cmd.AddValue ("simTime", "Simulation time in seconds. Default: 10s", simulationTime);
  cmd.AddValue ("testArg", "Extra simulation argument. Default: 0", testArg);
  //cmd.Parse (argc, argv);





uint16_t numberOfRandomUes =100;
  double simTime = 5;
  uint8_t bandwidth = 25;
  double radius = 1000.0;
  cmd.AddValue ("numberOfRandomUes", "Number of UEs", numberOfRandomUes);
  cmd.AddValue ("simTime", "Total duration of the simulation (in seconds)", simTime);
cmd.Parse (argc, argv);

//Input the parameters from the config txt file
ConfigStore inputConfig;
inputConfig.ConfigureDefaults ();

// parse again so you can override default values from the command line
cmd.Parse (argc, argv);

 Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (bandwidth));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (bandwidth));

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());

  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer enbNodes;
  NodeContainer ueNodes;

  enbNodes.Create (7);
  ueNodes.Create (numberOfRandomUes);

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector (0.0, 0.0, 1000.0));                       // eNB1 (0,0,0)
  enbPositionAlloc->Add (Vector (-1.732/2.0*radius,  1.5*radius, 1000.0));                 // eNB2 
  enbPositionAlloc->Add (Vector (1.732/2.0*radius, 1.5 * radius, 1000.0));   // eNB3 
  enbPositionAlloc->Add (Vector (1.732*radius, 0.0, 1000.0));  //eNB4
  enbPositionAlloc->Add (Vector (1.732/2.0*radius, -1.5*radius, 1000.0));  //eNB5
  enbPositionAlloc->Add (Vector (-1.732/2.0*radius, -1.5*radius, 1000.0));  //eNB6
  enbPositionAlloc->Add (Vector (-1.732*radius, 0.0, 1000.0));  //eNB7
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (enbPositionAlloc);
  mobility.Install (enbNodes);

  Ptr<RandomBoxPositionAllocator> randomUePositionAlloc = CreateObject<RandomBoxPositionAllocator> ();
  Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable> ();
  xVal->SetAttribute ("Min", DoubleValue (-1500.0));
  xVal->SetAttribute ("Max", DoubleValue (1500.0));
  randomUePositionAlloc->SetAttribute ("X", PointerValue (xVal));
  Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable> ();
  yVal->SetAttribute ("Min", DoubleValue (-1500.0));
  yVal->SetAttribute ("Max", DoubleValue (1500.0));
  randomUePositionAlloc->SetAttribute ("Y", PointerValue (yVal));
  Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable> ();
  zVal->SetAttribute ("Min", DoubleValue (500.0));
  zVal->SetAttribute ("Max", DoubleValue (1500.0));
  randomUePositionAlloc->SetAttribute ("Z", PointerValue (zVal));
  mobility.SetPositionAllocator (randomUePositionAlloc);
  mobility.Install (ueNodes);

  NetDeviceContainer enbDevs;
  
  NetDeviceContainer ueDevs;
      
  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);

  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

  std::cout<<" simTime: "<<simTime <<" | all ues is: "<<ueNodes.GetN ()<<std::endl;

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  lteHelper->AttachToClosestEnb (ueDevs, enbDevs);

  uint16_t dlPort = 10000;
  uint16_t ulPort = 20000;

  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.010));
  Ptr<UniformRandomVariable> stopTimeSeconds = CreateObject<UniformRandomVariable> ();
  stopTimeSeconds->SetAttribute ("Min", DoubleValue (0.015));
  stopTimeSeconds->SetAttribute ("Max", DoubleValue (0.050));

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      for (uint32_t b = 0; b < 1; ++b)
        {
          ++dlPort;
          ++ulPort;

          ApplicationContainer clientApps;
          ApplicationContainer serverApps;

          UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
          dlClientHelper.SetAttribute ("MaxPackets", UintegerValue (1000000));
          dlClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (10.0)));
          clientApps.Add (dlClientHelper.Install (remoteHost));
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), dlPort));
          serverApps.Add (dlPacketSinkHelper.Install (ue));


          UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
          ulClientHelper.SetAttribute ("MaxPackets", UintegerValue (1000000));
          ulClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (10.0)));
          clientApps.Add (ulClientHelper.Install (ue));
          PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

          Ptr<EpcTft> tft = Create<EpcTft> ();
          EpcTft::PacketFilter dlpf;
          dlpf.localPortStart = dlPort;
          dlpf.localPortEnd = dlPort;
          tft->Add (dlpf);

          EpcTft::PacketFilter ulpf;
          ulpf.remotePortStart = ulPort;
          ulpf.remotePortEnd = ulPort;
          tft->Add (ulpf);
          if (u%2==0)
          {
          EpsBearer bearer (EpsBearer::GBR_GAMING);
          lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), bearer, tft);
          }
          else
          {
          EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
          lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), bearer, tft);
          }

          Time startTime = Seconds (startTimeSeconds->GetValue ());
          Time stopTime = Seconds (stopTimeSeconds->GetValue ());
          serverApps.Start (startTime);
          clientApps.Start (startTime);
          serverApps.Stop(stopTime);
          clientApps.Stop(stopTime);
        }
    }

  lteHelper->EnableTraces ();
  lteHelper->EnableMacTraces();

  

 


 
  // OpenGym Env
  Ptr<OpenGymInterface> openGym = CreateObject<OpenGymInterface> (openGymPort);
  openGym->SetGetActionSpaceCb( MakeCallback (&MyGetActionSpace) );
  openGym->SetGetObservationSpaceCb( MakeCallback (&MyGetObservationSpace) );
  openGym->SetGetGameOverCb( MakeCallback (&MyGetGameOver) );
  openGym->SetGetObservationCb( MakeCallback (&MyGetObservation) );
  openGym->SetGetRewardCb( MakeCallback (&MyGetReward) );
  openGym->SetGetExtraInfoCb( MakeCallback (&MyGetExtraInfo) );
  openGym->SetExecuteActionsCb( MakeCallback (&MyExecuteActions) );
  Simulator::Schedule (Seconds(0.0), &ScheduleNextStateRead, envStepTime, openGym);
  NS_LOG_UNCOND ("Simulation start");
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  
  NS_LOG_UNCOND ("Simulation stop");

  openGym->NotifySimulationEnd();
  Simulator::Destroy ();
  return 0;
}
