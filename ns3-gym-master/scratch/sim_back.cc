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





//NS_LOG_FUNCTION_NOARGS ();
//Number of Users
uint16_t m_nUser = 50;
uint16_t n_enb = 7;
//Distance
double distance = 60;

double interPacketInterval = 20;

//Set the simulation time
double simTime = simulationTime;

// Command line arguments
//CommandLine cmd;
cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", m_nUser);
cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
cmd.AddValue("distance", "Distance between eNBs [m]", distance);
cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
cmd.Parse (argc, argv);

//Input the parameters from the config txt file
ConfigStore inputConfig;
inputConfig.ConfigureDefaults ();

// parse again so you can override default values from the command line
cmd.Parse (argc, argv);

  //Run the input-defaults.txt document
  // ./waf --command-template="%s --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load --ns3::ConfigStore::FileFormat=RawText" --run scratch/firstprogram

  //Comment?
  RngSeedManager::SetSeed (5);
  RngSeedManager::SetRun (4);


  //Set some of the Bit Error Stuff
  double ber = 0.000001;
  Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::PiroEW2010));
  Config::SetDefault ("ns3::LteAmc::Ber", DoubleValue (ber));
  //The next 2 commands disable the usage of the phy error model
  //Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (false));
  //Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping",EnumValue(LteEnbRrc::RLC_UM_ALWAYS));
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity",UintegerValue (320));

  //Set LTE Helper and EPC Helper
  //Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (25));//6 15 25  50 75 100
	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (25));

  //Have not set the antenna model type  
  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
  


  //Pathloss/Fading Model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisSpectrumPropagationLossModel"));


  //Create the Packet Gateway
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("50Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (1.0)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  //Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  
  //Create Nodes: eNodeB and UE
  NodeContainer enbNodes;
  NodeContainer ueNodes;
  enbNodes.Create (n_enb);
  ueNodes.Create (m_nUser);

  int dis = 500;
  Ptr<UniformRandomVariable> z = CreateObject<UniformRandomVariable> ();
  z->SetAttribute ("Min", DoubleValue (-dis));
  z->SetAttribute ("Max", DoubleValue (dis));
  Ptr<RandomRectanglePositionAllocator> alc = CreateObject<RandomRectanglePositionAllocator> ();
  alc->SetX(z);
  alc->SetY(z);


  //Set UEs position
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (int i=0; i < m_nUser; i++)
          {
      positionAlloc->Add (Vector(50 * (i-1)-300, 0, 0));  //scenario 2: different distance
      
          }
          
  // Install Mobility Model
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //                             "Bounds", RectangleValue (Rectangle (-250, 250, -250, 250)));
  mobility.Install (enbNodes);
  //mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  enbNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector(0,0,100));
  enbNodes.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector(dis/2,0,100));
    enbNodes.Get (6)->GetObject<MobilityModel> ()->SetPosition (Vector(dis/4,dis*1.41/2,100));
  enbNodes.Get (5)->GetObject<MobilityModel> ()->SetPosition (Vector(-dis/4,dis*1.41/2,100));
    enbNodes.Get (4)->GetObject<MobilityModel> ()->SetPosition (Vector(-dis/2,0,100));
  enbNodes.Get (3)->GetObject<MobilityModel> ()->SetPosition (Vector(-dis/4,-dis*1.41/2,100));
    enbNodes.Get (2)->GetObject<MobilityModel> ()->SetPosition (Vector(dis/4,-dis*1.41/2,100));
  //mobility.SetPositionAllocator(positionAlloc);

  // mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  //                                 "MinX", DoubleValue (-5.0),
  //                                 "MinY", DoubleValue (-5.0),
  //                                 "DeltaX", DoubleValue (2.0),
  //                                 "DeltaY", DoubleValue (2.0),
  //                                 "GridWidth", UintegerValue (3),
  //                                 "LayoutType", StringValue ("RowFirst"));
  // mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ueNodes);
  
// ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector(0,0,0));
//   ueNodes.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector(250,0,0));
//     ueNodes.Get (2)->GetObject<MobilityModel> ()->SetPosition (Vector(125,215,0));
//   ueNodes.Get (3)->GetObject<MobilityModel> ()->SetPosition (Vector(-125,215,0));
//     ueNodes.Get (4)->GetObject<MobilityModel> ()->SetPosition (Vector(-250,0,0));
//   ueNodes.Get (5)->GetObject<MobilityModel> ()->SetPosition (Vector(-125,-215,0));
//     ueNodes.Get (6)->GetObject<MobilityModel> ()->SetPosition (Vector(125,-215,0));
//     ueNodes.Get (7)->GetObject<MobilityModel> ()->SetPosition (Vector(-50,0,0));
//   ueNodes.Get (8)->GetObject<MobilityModel> ()->SetPosition (Vector(225,0,0));
//     ueNodes.Get (9)->GetObject<MobilityModel> ()->SetPosition (Vector(110,215,0));
//   ueNodes.Get (10)->GetObject<MobilityModel> ()->SetPosition (Vector(-110,215,0));
//     ueNodes.Get (11)->GetObject<MobilityModel> ()->SetPosition (Vector(-225,0,0));
//   ueNodes.Get (12)->GetObject<MobilityModel> ()->SetPosition (Vector(-135,-215,0));
//     ueNodes.Get (13)->GetObject<MobilityModel> ()->SetPosition (Vector(135,-215,0));

  for (uint16_t i = 0; i < m_nUser; i++)
  {
    ueNodes.Get (i)->GetObject<MobilityModel> ()->SetPosition (alc->GetNext());
    // ueNodes.Get (i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));
  }

  
  //NetDeviceContainer enbDevs = lteHelper->InstallEnbDevice (enbNodes.Get(0));
  // std::string frAlgorithmType = lteHelper->GetFfrAlgorithmType ();
  // //NS_LOG_DEBUG ("FrAlgorithmType: " << frAlgorithmType);
  // lteHelper->SetFfrAlgorithmType ("ns3::LteFrSoftAlgorithm");
  // if (frAlgorithmType == "ns3::LteFrHardAlgorithm")
  //   {

  //     //Nothing to configure here in automatic mode

  //   }
  // else if (frAlgorithmType == "ns3::LteFrStrictAlgorithm")
  //   {

  //     lteHelper->SetFfrAlgorithmAttribute ("RsrqThreshold", UintegerValue (32));
  //     lteHelper->SetFfrAlgorithmAttribute ("CenterPowerOffset",
  //                                          UintegerValue (LteRrcSap::PdschConfigDedicated::dB_6));
  //     lteHelper->SetFfrAlgorithmAttribute ("EdgePowerOffset",
  //                                          UintegerValue (LteRrcSap::PdschConfigDedicated::dB3));
  //     lteHelper->SetFfrAlgorithmAttribute ("CenterAreaTpc", UintegerValue (0));
  //     lteHelper->SetFfrAlgorithmAttribute ("EdgeAreaTpc", UintegerValue (3));

  //     //ns3::LteFrStrictAlgorithm works with Absolute Mode Uplink Power Control
  //     Config::SetDefault ("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue (false));

  //   }
  // else if (frAlgorithmType == "ns3::LteFrSoftAlgorithm")
  //   {

  //     lteHelper->SetFfrAlgorithmAttribute ("AllowCenterUeUseEdgeSubBand", BooleanValue (true));
  //     lteHelper->SetFfrAlgorithmAttribute ("RsrqThreshold", UintegerValue (25));
  //     lteHelper->SetFfrAlgorithmAttribute ("CenterPowerOffset",
  //                                          UintegerValue (LteRrcSap::PdschConfigDedicated::dB_6));
  //     lteHelper->SetFfrAlgorithmAttribute ("EdgePowerOffset",
  //                                          UintegerValue (LteRrcSap::PdschConfigDedicated::dB3));
  //     lteHelper->SetFfrAlgorithmAttribute ("CenterAreaTpc", UintegerValue (0));
  //     lteHelper->SetFfrAlgorithmAttribute ("EdgeAreaTpc", UintegerValue (3));

  //     //ns3::LteFrSoftAlgorithm works with Absolute Mode Uplink Power Control
  //     Config::SetDefault ("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue (false));

  //   }
  // else if (frAlgorithmType == "ns3::LteFfrSoftAlgorithm")
  //   {

  //     lteHelper->SetFfrAlgorithmAttribute ("CenterRsrqThreshold", UintegerValue (30));
  //     lteHelper->SetFfrAlgorithmAttribute ("EdgeRsrqThreshold", UintegerValue (25));
  //     lteHelper->SetFfrAlgorithmAttribute ("CenterAreaPowerOffset",
  //                                          UintegerValue (LteRrcSap::PdschConfigDedicated::dB_6));
  //     lteHelper->SetFfrAlgorithmAttribute ("MediumAreaPowerOffset",
  //                                          UintegerValue (LteRrcSap::PdschConfigDedicated::dB_1dot77));
  //     lteHelper->SetFfrAlgorithmAttribute ("EdgeAreaPowerOffset",
  //                                          UintegerValue (LteRrcSap::PdschConfigDedicated::dB3));
  //     lteHelper->SetFfrAlgorithmAttribute ("CenterAreaTpc", UintegerValue (1));
  //     lteHelper->SetFfrAlgorithmAttribute ("MediumAreaTpc", UintegerValue (2));
  //     lteHelper->SetFfrAlgorithmAttribute ("EdgeAreaTpc", UintegerValue (3));

  //     //ns3::LteFfrSoftAlgorithm works with Absolute Mode Uplink Power Control
  //     Config::SetDefault ("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue (false));

  //   }
  // else if (frAlgorithmType == "ns3::LteFfrEnhancedAlgorithm")
  //   {

  //     lteHelper->SetFfrAlgorithmAttribute ("RsrqThreshold", UintegerValue (25));
  //     lteHelper->SetFfrAlgorithmAttribute ("DlCqiThreshold", UintegerValue (10));
  //     lteHelper->SetFfrAlgorithmAttribute ("UlCqiThreshold", UintegerValue (10));
  //     lteHelper->SetFfrAlgorithmAttribute ("CenterAreaPowerOffset",
  //                                          UintegerValue (LteRrcSap::PdschConfigDedicated::dB_6));
  //     lteHelper->SetFfrAlgorithmAttribute ("EdgeAreaPowerOffset",
  //                                          UintegerValue (LteRrcSap::PdschConfigDedicated::dB3));
  //     lteHelper->SetFfrAlgorithmAttribute ("CenterAreaTpc", UintegerValue (0));
  //     lteHelper->SetFfrAlgorithmAttribute ("EdgeAreaTpc", UintegerValue (3));

  //     //ns3::LteFfrEnhancedAlgorithm works with Absolute Mode Uplink Power Control
  //     Config::SetDefault ("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue (false));

  //   }
  // else if (frAlgorithmType == "ns3::LteFfrDistributedAlgorithm")
  //   {

  //     NS_FATAL_ERROR ("ns3::LteFfrDistributedAlgorithm not supported in this example. Please run lena-distributed-ffr");

  //   }
  // else
  //   {
  //     lteHelper->SetFfrAlgorithmType ("ns3::LteFrNoOpAlgorithm");
  //   }


 
  


  //Create Devices and install them in the Nodes (eNB and UE)
  // lteHelper->SetFfrAlgorithmType("ns3::LteFfrSoftAlgorithm");
  // lteHelper->SetFfrAlgorithmAttribute("FrCellTypeId", UintegerValue (1));
  // NetDeviceContainer enbDevs;
  // NetDeviceContainer ueDevs;
  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);
  for (int i=0; i < m_nUser; i++)
          {
        Ptr<LteUeNetDevice> lteUeDev = ueDevs.Get (i)->GetObject<LteUeNetDevice> ();
        Ptr<LteUePhy> uePhy = lteUeDev->GetPhy ();
        uePhy->SetAttribute ("TxPower", DoubleValue (23.0));
        uePhy->SetAttribute ("NoiseFigure", DoubleValue (5.0 ));
      }

  //Set eNBs power
  for (int i=0; i < n_enb; i++)
          {		
  Ptr<LteEnbNetDevice> lteEnbDev = enbDevs.Get (i)->GetObject<LteEnbNetDevice> ();
  Ptr<LteEnbPhy> enbPhy = lteEnbDev->GetPhy ();
  enbPhy->SetAttribute ("TxPower", DoubleValue (50.0));
  enbPhy->SetAttribute ("NoiseFigure", DoubleValue (5.0));
          }


  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
      {
        Ptr<Node> ueNode = ueNodes.Get (u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
      }


     
  lteHelper->AttachToClosestEnb (ueDevs, enbDevs);
  //Activate an EPS bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  //The DataRadioBearer was activated automatically in a previous step
  //The following command would give an error if uncommented
  //lteHelper->ActivateDataRadioBearer (ueDevs, bearer);
  //Side effect: the default EPS bearer will be activated


  // UDP connection from remote to UEnodes
    uint16_t dlPort = 1234;

    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
      {
        dlPort++;
        PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));

        serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));

        UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
        dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
        dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

        clientApps.Add (dlClient.Install (remoteHost));

      }
    serverApps.Start (Seconds (0.01));
    Ptr<ExponentialRandomVariable> ktime = CreateObject<ExponentialRandomVariable> ();
    ktime->SetAttribute ("Mean", DoubleValue (2));
    ktime->SetAttribute ("Bound", DoubleValue (5));
    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
      {

        
        double st = (double)possion(15,U_Random(u))/100;
          cout << st+1 << endl;
          double kt = ktime->GetValue();
          cout << kt << endl;
         

        
        clientApps.Get(u)->SetStartTime(Seconds (st));
        clientApps.Get(u)->SetStopTime(Seconds (st+kt));
      }
    lteHelper->EnableMacTraces ();
    lteHelper->EnableRlcTraces ();


    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.Install(ueNodes);
    flowmon.InstallAll ();


    Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
    rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (simTime)));

  NS_LOG_UNCOND("Ns3Env parameters:");
  NS_LOG_UNCOND("--simulationTime: " << simulationTime);
  NS_LOG_UNCOND("--openGymPort: " << openGymPort);
  NS_LOG_UNCOND("--envStepTime: " << envStepTime);
  NS_LOG_UNCOND("--seed: " << simSeed);
  NS_LOG_UNCOND("--testArg: " << testArg);

  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (simSeed);
 
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
  monitor->CheckForLostPackets (); 

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

    double Throughput=0.0;

    for (map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
      {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);

        cout << "Flow ID: " << i->first << " Src Addr " <<  t.sourceAddress << " Dst Addr " << t.destinationAddress << endl;
        cout << "Tx Packets = " << i->second.txPackets << endl;
        cout << "Rx Packets = " << i->second.rxPackets << endl;
        Throughput=i->second.rxBytes * 8.0 /(i->second.timeLastRxPacket.GetSeconds()-i->second.timeFirstTxPacket.GetSeconds())/ 1024;
        cout << "Throughput: " <<  Throughput << " Kbps" << endl;
      }

  NS_LOG_UNCOND ("Simulation stop");

  openGym->NotifySimulationEnd();
  Simulator::Destroy ();

}
