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
#include <ns3/epc-mme.h>
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
#include <iostream>
//FlowMonitor
#include "ns3/flow-monitor-module.h"
using namespace ns3;
using namespace std;


uint16_t ttt=2;
uint16_t zz=2;
int num_ue_ = 0;
list<list<int>> ls;
list<list<int>> delay;
double delay_budget[] = {100,150,50,300,100,300,100,300,100}; 
// Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();

NetDeviceContainer enbDevs;
NetDeviceContainer ueDevs;
NS_LOG_COMPONENT_DEFINE ("OpenGym");

/*
split函数，分割字符串
 */
std::vector<std::string>
split (const char *s, const char *delim)
{
  std::vector<std::string> result;
  if (s && strlen (s))
    {
      int len = strlen (s);
      char *src = new char[len + 1];
      strcpy (src, s);
      src[len] = '\0';
      char *tokenptr = strtok (src, delim);
      while (tokenptr != NULL)
        {
          std::string tk = tokenptr;
          result.emplace_back (tk);
          tokenptr = strtok (NULL, delim);
        }
      delete[] src;
    }
  return result;
}
/*
Define observation space
用于gym从ns3获取状态空间
未使用此函数
*/
Ptr<OpenGymSpace>
MyGetObservationSpace (void)
{
  
  cout << "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+" << endl;

  uint32_t nodeNum = 5;
  float low = 0.0;
  float high = 10.0;
  std::vector<uint32_t> shape = {
      nodeNum,
  };
  std::string dtype = TypeNameGet<uint32_t> ();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace> (low, high, shape, dtype);

  return space;
}

/*
Define action space
用于gym从ns3获取动作空间
未使用此函数
*/
Ptr<OpenGymSpace>
MyGetActionSpace (void)
{
  Ptr<OpenGymTupleSpace> space = CreateObject<OpenGymTupleSpace> ();
  for (int i = 0; i < 1000; i++)
    space->Add (CreateObject<OpenGymDiscreteSpace> (1));

  return space;
}

/*
Define game over condition
用于传递仿真结束的标志
*/
bool
MyGetGameOver (void)
{

  bool isGameOver = false;
  NS_LOG_UNCOND ("GameOver: " << isGameOver);

  return isGameOver;
}

/*
Collect observations
用于从ns3中统计环境信息，封装到OpenGymBoxContainer中
*/
Ptr<OpenGymDataContainer>
MyGetObservation (void)
{

  static float stepCounter = -1; //step计数
  stepCounter += 1;
  cout << "step: " << stepCounter << endl;
  uint32_t nodeNum = 7;
  std::vector<uint32_t> shape = {
      nodeNum,
  };
  Ptr<OpenGymBoxContainer<uint32_t>> box = CreateObject<OpenGymBoxContainer<uint32_t>> (
      shape); //创建OpenGymBoxContainer用以包装存储observation，进而传递给gym
  

  ls.clear();
  // cout << ueDevs.Get(0)-> GetObject<LteEnbNetDevice>()->GetCellId() << endl;
  // // ueDevs.Get(0)-> GetObject<LteUeNetDevice>() ->GetPhy() ->
  // cout << ueDevs.Get(0)-> GetObject<LteUeNetDevice>() ->GetImsi() <<  endl;
  // 获取每个波束下有数据传输需求的ue的RNTI以及需要传输的数据量，依照等待传输的数据量多少进行排序
 
  // std::map<uint64_t, Ptr<ns3::EpcMme::UeInfo> >::iterator eui = epcHelper->m_mme->m_ueInfoMap.begin();
  
  // for(eui = epcHelper->m_mme->m_ueInfoMap.begin(); eui != epcHelper->m_mme->m_ueInfoMap.end(); eui++)
  // {

  
  // for(std::list<ns3::EpcMme::BearerInfo>::iterator mmp = eui->second->bearersToBeActivated.begin();mmp != eui->second->bearersToBeActivated.end(); mmp++)
  // {
  //   cout << mmp->bearer.qci << endl;
  // } 
  // }
  for (uint16_t i = 0; i < 7; i++)
    {
      cout << "---------------------------------------" << endl;
      // cout << "cellid: " << i << endl; 

  //     Ptr<LteEnbRrc> enbRrc = enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetRrc ();
  //     std::map<uint16_t, Ptr<UeManager> >::iterator ppp; 
  //     cout << enbRrc->m_ueMap.size() << endl;
  //     cout << "------------------------" << endl;
  //     for(ppp = enbRrc->m_ueMap.begin(); ppp != enbRrc->m_ueMap.end();ppp++)
  //     {
  //       cout << ppp->first << endl;
  //     }






      PointerValue ptr;
      enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetCcMap ().at (0)->GetAttribute (
          "FfMacScheduler", ptr);
     Ptr<LteEnbRrc> enbRrc = enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetRrc ();

      Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
      Ptr<RrFfMacScheduler> pff = ff->GetObject<RrFfMacScheduler> ();
      pff->RefreshDlCqiMaps();
      std::map <uint16_t,uint8_t>::iterator p10it;
      cout << "p10" << endl;

      cout << "size:" << pff-> m_p10CqiRxed.size() << endl;
      for(p10it = pff-> m_p10CqiRxed.begin();p10it != pff-> m_p10CqiRxed.end();p10it++)
      {
        // cout << "RNTI: " << p10it -> first << endl;
        // cout << "p10Cqi: " << (u_int16_t)(p10it -> second) << endl;
        // cout << "---------------" << endl;

      }

      // std::map <uint16_t,SbMeasResult_s>::iterator a30it;
      // cout << "a30" << endl;
      // // cout << "size:" << pff-> m_a30CqiRxed.size() << endl;
      // for(a30it = pff-> m_a30CqiRxed.begin();a30it != pff-> m_a30CqiRxed.end();a30it++)
      // {
      //   cout << "RNTI:" << a30it -> first << endl;
        
      //   cout << "bwcqi:" <<(u_int16_t)(a30it -> second.m_bwPart.m_cqi) << " ";
      //   if(a30it -> second.m_higherLayerSelected.begin()->m_sbCqi.size() > 0 )
      //   {
      //     cout << "higherLayerSelected_cqi:" << (u_int16_t)(*(a30it -> second.m_higherLayerSelected.begin()->m_sbCqi.begin()) )<< " ";
      //   }
      //   if(a30it -> second.m_ueSelected.m_sbCqi.size()>0){
      //     cout << "ueSelected:" << (u_int16_t)(*(a30it -> second.m_ueSelected.m_sbCqi.begin()) )<< " ";
      //   }
      //   cout << endl;
      //   cout << "---------------" << endl;

      // }

      for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
               it = pff->m_rlcBufferReq_1.begin ();
           it != pff->m_rlcBufferReq_1.end (); it++)
        {
          //   if(enbRrc->HasUeManager(it->first.m_rnti))
          // {
          //   std::map <uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator pdrb = enbRrc->GetUeManager(it->first.m_rnti)->m_drbMap.begin();
          //   cout << "===========" << endl;
          //   cout << "RNTI:" << (uint16_t)(it->first.m_rnti) << endl;
          //   cout << "qci:" << (++pdrb)->second->m_epsBearer.qci << endl;
          //   // cout << (uint16_t)(pdrb->first) << endl;
            
          //  }

          list<int> temp1;
          


          list<int> temp;
          if (it->first.m_rnti != 0 && it->second.m_rlcTransmissionQueueSize != 0)
            {
              temp.push_back ((u_int16_t)(pff-> m_p10CqiRxed.find(it->first.m_rnti) ->second));
              
              std::map <uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator pdrb = enbRrc->GetUeManager(it->first.m_rnti)->m_drbMap.begin();
              temp.push_back ((++pdrb)->second->m_epsBearer.qci);
              temp.push_back (it->second.m_rlcTransmissionQueueSize);
              
              temp.push_back (i);
              temp.push_back (it->first.m_rnti);
              int flag = 1;
              for(list<list<int>>::iterator ia = delay.begin(); ia != delay.end(); ia++)
              {
                // if(*(ia->begin()) == i && pff->m_rlcBufferReq_1.find(*(++(ia->begin()))) == pff->m_rlcBufferReq_1.end())
                // {
                //   *(++(++(ia->begin()))) = 0;
                // }
                if(*(ia->begin()) == i && *(++(ia->begin())) == it->first.m_rnti)
                {
                  flag = 0;
                  break;
                }
              }
              if(flag == 1)
              {
                temp1.push_back(i);
                temp1.push_back(it->first.m_rnti);
                temp1.push_back(0);
                temp1.push_back(0);
              }

            }
          if(!temp.empty())
            ls.push_back (temp);
          if(!temp1.empty())
            delay.push_back(temp1);
        }
      // ls.sort (); //依等待传输的数据量排序
      // ls.reverse (); //上面是从小到大，故进行逆序
    }
  uint16_t num_ue = 0; //统计总的业务请求数
  for (list<list<int>>::iterator it = ls.begin (); it != ls.end () && num_ue < 84; it++)
    {
      for (list<int>::iterator iz = it->begin (); iz != it->end (); iz++)
        {
          if (iz == it->begin ())
          {
            box->AddValue (*(iz));
            box->AddValue (*(++iz));
            box->AddValue (*(++iz));
          }
             
        }
      num_ue++;
    }

  num_ue_ = ls.size ();

  //如果请求数不足84，则进行补0

  if (ls.size () <= 84)
    {
      for (uint16_t i = 0; i < 84 - ls.size (); i++)
        {
          box->AddValue (0);
          box->AddValue (0);
        }
    }
  else
    {
    }

  cout << "Observation:(排序后) \n" << box << endl;

  return box;
}

/*
Define extra info. Optional
用于传递其他信息，暂未使用
*/
std::string
MyGetExtraInfo (void)
{
  std::string myInfo = "testInfo";
  myInfo += "|123";
  NS_LOG_UNCOND ("ExtraInfo: " << myInfo);
  return myInfo;
}
// void
// tt()
// {
//   lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
// }
/*
Execute received actions
ns3接收到从gym中传递过来的封装在OpenGymDataContainer中的动作action，并将其以适当的形式作为参数传递到调度器中执行
*/
void tt()
{
  lteHelper->SetSchedulerType("ns3::FdBetFfMacScheduler");
  cout << "*-*-*-**-*--------------------------------------------" << endl;
}
bool
MyExecuteActions (Ptr<OpenGymDataContainer> action)
{
  // tt();
  
 
  Ptr<OpenGymTupleContainer> tu_ac = DynamicCast<OpenGymTupleContainer> (
      action); //将传递过来的action自动转换为OpenGymTupleContainer
  // //传递过来的OpenGymTupleContainer封装的动作的形式是(CellID1,RNTI1,资源编号1,CellID2,RNTI2,资源编号2,CellID3,RNTI3,资源编号3,……)
  // //从OpenGymTupleContainer中读取数据并保存到字符串中
  // string ac_s[7] = {"", "", "", "", "", "", ""};
  // for (uint32_t i = 0; i < tu_ac->size(); i += 3)
  //   {
  //     //如果没有动作，标记为9999
  //     if (DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 2))->GetValue () == 9999)
  //       {
  //         ac_s[0] += to_string (9999);
  //         ac_s[1] += to_string (9999);
  //         ac_s[2] += to_string (9999);
  //         ac_s[3] += to_string (9999);
  //         ac_s[4] += to_string (9999);
  //         ac_s[5] += to_string (9999);
  //         ac_s[6] += to_string (9999);
  //       }
  //     else
  //       {
  //         //第i个波束（eNB）的调度信息保存为 RNTI1 资源编号1 RNTI2 资源编号2 RNTI3 资源编号3 …… 形式的字符串
  //         ac_s[DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i))->GetValue ()] +=
  //             (to_string (DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 1))->GetValue ()) +
  //              " " +
  //              to_string (DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 2))->GetValue ()));
  //         ac_s[DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i))->GetValue ()] += " ";
  //       }
  //   }

  // NS_LOG_UNCOND ("Action: \n" << tu_ac);
  // //以字符串的形式将动作传递到各个波束（eNB）进行调度
  // for (uint16_t i = 0; i < 7; i++)
  //   {
      
  //     PointerValue ptr;
  //     enbDevs.action_listGet (i)->GetObject<LteEnbNetDevice> ()->GetCcMap ().at (0)->GetAttribute (
  //         "FfMacScheduler", ptr); //获取波束(eNB)i的调度器
  //     Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
  //     // ac_s[i] = to_string(i) + " " + ac_s[i]; 
  //     // cout << "===" << ac_s[i].size() << endl;
  //     StringValue c;
  //     //通过SetAttribute将动作参数传到调度器中
  //     ff->SetAttribute ("test", StringValue (ac_s[i]));
  //     ff->SetAttribute ("cellid", StringValue (to_string(i)));
  //     //分别打印各个波束的调度信息
  //     ff->GetAttribute ("test", c);
  //     vector<string> vcs = split (c.Get ().c_str (), " ");
  //     // string res_alloc = "";

  //     // for (uint16_t j = 0; j < vcs.size (); j += 2)
  //     //   {
  //     //     res_alloc += vcs[j];
  //     //     res_alloc += ":";
  //     //     res_alloc += vcs[j + 1];
  //     //     res_alloc += " ";
  //     //   }
  //     // cout << "第" << i + 1 << "个波束分配情况"
  //         //  << "(RNTI:资源编号):" << endl;
  //     // cout << res_alloc << endl;
  //   }
  // cout << "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+" << endl;





  // ttt = ttt + 1;
  // zz = zz + 2;
  uint8_t rs = DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (0))->GetValue ();
  for (uint16_t i = 0; i < 7; i++)
    {
      // cout << "=-=-=0" << endl;
      PointerValue ptr;
      enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetCcMap ().at (0)->GetAttribute (
          "FfMacScheduler", ptr); //获取波束(eNB)i的调度器
      Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
      // ac_s[i] = to_string(i) + " " + ac_s[i]; 
      // cout << "===" << ac_s[i].size() << endl;
      StringValue c;
      //通过SetAttribute将动作参数传到调度器中
      // uint8_t cv = 1;//ttt%4;
      // if(cv == 2)
      //   cv++;
      ff->SetAttribute ("m_flag", UintegerValue (rs));
      // cout << ttt%3 << "=--=00--=" << endl;
      // ff->SetAttribute ("cellid", StringValue (to_string(i)));
    }
  
  
  return true;
}


/*
Define reward function
用于gym从ns3中获取动作执行后的反馈信息
*/
float
MyGetReward (void)
{
  cout << "执行reward" << endl;
  float reward = 0.0;
  //reward = 1;
  ifstream file;
  file.open ("/home/liqi/ns3-gym/ns3-gym-master/DlRxPhyStats.txt"); //打开系统生成的日志
  char szbuff[1024] = {0};
  string za;
  // float txx = 0;
  // float zx = 0;
  Time tim = Simulator::Now (); //获取系统当前时刻
  // float tobn = 0;       //总发包数
  // float rxbn = 0;       //成功接收到的包数
  // float blossrate = 0;  //丢包率
  // float throught = 0; //吞吐量
  file.getline (szbuff, 1024);
  while (!file.eof ())
    {
      // cout << "-09-090" << endl;
      // tobn += 1;
      file.getline (szbuff, 1024);
      za = szbuff;
      
      vector<string> sp = split (za.c_str (), "\t");
      //获取系统上个时刻的日志信息，得到需要的数据
      // if (sp.size () == 12 && atoi (sp[0].c_str ()) == (tim.GetMilliSeconds () - 1) &&
      //     atoi (sp[10].c_str ()) == 1)
      //   {
      //     // txx += atoi (sp[7].c_str ());
      //     // rxbn += 1;
      //     // zx++;
      //   }
      for(list<list<int>>::iterator it1 = delay.begin(); it1 != delay.end(); it1++)
      {
        list<int>::iterator zi1 = it1->begin();
        if(sp.size () == 12 && *(zi1) == atoi(sp[1].c_str ()) &&  *(++zi1) == atoi(sp[3].c_str ())&& atoi (sp[0].c_str ()) == (tim.GetMilliSeconds () - 1))   
        {
          *(++zi1) = 0;
          *(++zi1) = 1;
        } 
        
      }
      for(list<list<int>>::iterator it = ls.begin(); it != ls.end(); it++)
      {
        list<int>::iterator zi = it->begin();
        // cout << (*(zi) == 1) << endl;
        // cout << (atoi(sp[1].c_str ()) == 1) << endl;
        
        
        // cout << "--------------" << endl;
        if(sp.size () == 12 && *(++(++(++zi))) == atoi(sp[1].c_str ()) &&  *(++zi) == atoi(sp[3].c_str ())&& atoi (sp[0].c_str ()) == (tim.GetMilliSeconds () - 1))
        {

          // cout << "+++++++++++++++++" << endl;
          // cout << reward << endl;
          reward += (0.7+0.3*(2*atoi (sp[10].c_str ())-1));
        }
      }
    }
     for(list<list<int>>::iterator it1 = delay.begin(); it1 != delay.end(); it1++)
      {
        list<int>::iterator zi1 = it1->begin();
        int d = 0;
        if(*(++(++(++zi1))) == 0)
        {
          d = ++(*(--zi1));
        } 
        else
        {
          *(zi1) = 0;
        }
        cout << d << endl;
        Ptr<LteEnbRrc> enbRrc = enbDevs.Get (*(it1->begin()))->GetObject<LteEnbNetDevice> ()->GetRrc ();
        std::map <uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator pdrb = enbRrc->GetUeManager(*(++(it1->begin())))->m_drbMap.begin();   
        uint16_t qci = (++pdrb)->second->m_epsBearer.qci;
        cout << qci << endl;
        reward += (-d/delay_budget[qci]);
        // cout << reward << "============" << endl;
      }
      cout << delay.size() << endl;
  // throught = txx / 0.001;
  // // blossrate = rxbn/tobn;

  // reward = zx;
  if (num_ue_ == 0)
    return 5.0;
  else
    {
      // cout << "用户数:" << num_ue_ << endl;
      return (reward);
    }
}
/*
ns3中的类似gym中的step函数
 */
void
ScheduleNextStateRead (double envStepTime, Ptr<OpenGymInterface> openGym)
{

  Simulator::Schedule (Seconds (envStepTime), &ScheduleNextStateRead, envStepTime, openGym);
  openGym->NotifyCurrentState ();
}

int
main (int argc, char *argv[])
{
  // Parameters of the scenario
  uint32_t simSeed = 1;
  double simulationTime = 10; //seconds
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

  uint16_t numberOfRandomUes = 100;
  uint8_t bandwidth = 25;
  double radius = 1000.0;
  cmd.AddValue ("numberOfRandomUes", "Number of UEs", numberOfRandomUes);

  //Input the parameters from the config txt file
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // parse again so you can override default values from the command line
  cmd.Parse (argc, argv);


  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (bandwidth));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (bandwidth));
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
  lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
  

  
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
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
      ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());

  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer enbNodes;
  NodeContainer ueNodes;

  enbNodes.Create (7);
  ueNodes.Create (numberOfRandomUes);

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector (0.0, 0.0, 1000.0)); // eNB1 (0,0,0)
  enbPositionAlloc->Add (Vector (-1.732 / 2.0 * radius, 1.5 * radius, 1000.0)); // eNB2
  enbPositionAlloc->Add (Vector (1.732 / 2.0 * radius, 1.5 * radius, 1000.0)); // eNB3
  enbPositionAlloc->Add (Vector (1.732 * radius, 0.0, 1000.0)); //eNB4
  enbPositionAlloc->Add (Vector (1.732 / 2.0 * radius, -1.5 * radius, 1000.0)); //eNB5
  enbPositionAlloc->Add (Vector (-1.732 / 2.0 * radius, -1.5 * radius, 1000.0)); //eNB6
  enbPositionAlloc->Add (Vector (-1.732 * radius, 0.0, 1000.0)); //eNB7
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (enbPositionAlloc);
  mobility.Install (enbNodes);

  Ptr<RandomBoxPositionAllocator> randomUePositionAlloc =
      CreateObject<RandomBoxPositionAllocator> ();
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

  //NetDeviceContainer enbDevs;

  //NetDeviceContainer ueDevs;

  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);

  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

  std::cout << " simTime: " << simulationTime << " | all ues is: " << ueNodes.GetN () << std::endl;

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  lteHelper->AttachToClosestEnb (ueDevs, enbDevs);

  uint16_t dlPort = 10000;
  uint16_t ulPort = 20000;

  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.010));
  Ptr<UniformRandomVariable> stopTimeSeconds = CreateObject<UniformRandomVariable> ();
  stopTimeSeconds->SetAttribute ("Min", DoubleValue (0.15));
  stopTimeSeconds->SetAttribute ("Max", DoubleValue (1));

  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
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
          
          if (u % 2 == 0)
            {
              enum EpsBearer::Qci q = EpsBearer::GBR_GAMING;
              EpsBearer bearer (q);
             
              lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), EpsBearer(EpsBearer:: GBR_CONV_VOICE ), tft);
              // lteHelper->ActivateDataRadioBearer(ueDevs.Get (u),EpsBearer(EpsBearer::NGBR_IMS));
            }
          else
            {
              // EpsBearer bearer (EpsBearer::GBR_GAMING);
              lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), EpsBearer(EpsBearer::NGBR_VOICE_VIDEO_GAMING), tft);
            }

          Time startTime = Seconds (startTimeSeconds->GetValue ());
          Time stopTime = Seconds (stopTimeSeconds->GetValue ());
          serverApps.Start (startTime);
          clientApps.Start (startTime);
          serverApps.Stop (stopTime);
          clientApps.Stop (stopTime);
        }
    }




  lteHelper->EnableTraces ();
  //lteHelper->EnableMacTraces();
  
  // OpenGym Env
  Ptr<OpenGymInterface> openGym = CreateObject<OpenGymInterface> (openGymPort);
  openGym->SetGetActionSpaceCb (MakeCallback (&MyGetActionSpace));
  openGym->SetGetObservationSpaceCb (MakeCallback (&MyGetObservationSpace));
  openGym->SetGetGameOverCb (MakeCallback (&MyGetGameOver));
  openGym->SetGetObservationCb (MakeCallback (&MyGetObservation));
  openGym->SetGetRewardCb (MakeCallback (&MyGetReward));
  openGym->SetGetExtraInfoCb (MakeCallback (&MyGetExtraInfo));
  openGym->SetExecuteActionsCb (MakeCallback (&MyExecuteActions));
  Simulator::Schedule (Seconds (0.0), &ScheduleNextStateRead, envStepTime, openGym);
  NS_LOG_UNCOND ("Simulation start");
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  // Simulator::Schedule(Seconds(0.01),&tt);
  NS_LOG_UNCOND ("Simulation stop");

  openGym->NotifySimulationEnd ();
  Simulator::Destroy ();
  cout << lteHelper->GetSchedulerType()<< endl;
  return 0;
}

