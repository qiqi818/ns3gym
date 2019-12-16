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
vector<double> delat_buget = {100,150,50,300,100,300,100,300,300};
float stepCounter = -1; //step计数

   
  // list<queue<map<>>>
  // list<list<double>> queue1;
  // list<list<double>> queue2;
  // list<list<double>> queue3;

  // double queue1 = 0.0;
  // double queue2 = 0.0;
  // double queue3 = 0.0;



  struct rParameters {
    uint32_t cellid;
    map<uint32_t, uint32_t> peruerbbitmap;
    map<uint32_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters> rlcbuffer;
    map<uint32_t, vector<double>> phyrxstates;
    map<uint32_t, vector<uint32_t>> uestates;
    map<uint32_t, SpectrumValue> sinr;
  };



list< vector< struct rParameters > > rewardParameters;

int num_ue_ = 0;
// Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
 FlowMonitorHelper flowmon;
Ptr<FlowMonitor> monitor;
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

void DlPhyReceptionCallback(Ptr<PhyRxStatsCalculator> phyRxStats,
                                              std::string path, PhyReceptionStatParameters params)
{
  cout << params.m_timestamp << endl;
  cout << "cellid: " << params.m_cellId << endl;
  cout << "rnti: " << params.m_rnti << endl;
  cout << params.m_size << endl;
  cout << "---------------------------" << endl;
}
Ptr<PhyRxStatsCalculator> m_phyRxStats = CreateObject<PhyRxStatsCalculator> ();
void tracetest()
{
  Config::Connect ("/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/LteUePhy/DlSpectrumPhy/DlPhyReception",
                   MakeBoundCallback (&DlPhyReceptionCallback, m_phyRxStats));
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
  for (int i = 0; i < 1200; i++)
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
  
  stepCounter += 1;
  cout << "step: " << stepCounter << endl;
  uint32_t nodeNum = 7;
  std::vector<uint32_t> shape = {
      nodeNum,
  };
  Ptr<OpenGymBoxContainer<uint32_t>> box = CreateObject<OpenGymBoxContainer<uint32_t>> (
      shape); //创建OpenGymBoxContainer用以包装存储observation，进而传递给gym
  list<list<int>> ls;


  // cout << ueDevs.Get(0)-> GetObject<LteEnbNetDevice>()->GetCellId() << endl;
  // ueDevs.Get(0)-> GetObject<LteUeNetDevice>() ->GetPhy() ->
  // cout << ueDevs.Get(0)-> GetObject<LteUeNetDevice>() ->GetImsi() <<  endl;
  //获取每个波束下有数据传输需求的ue的RNTI以及需要传输的数据量，依照等待传输的数据量多少进行排序
 
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
      // cout << "-----" << i << endl;

  //      Ptr<LteEnbRrc> enbRrc = enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetRrc ();
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
      Ptr<DacFfMacScheduler> pff = ff->GetObject<DacFfMacScheduler> ();
      // pff->RefreshDlCqiMaps();
      // std::map <uint16_t,uint8_t>::iterator p10it;
      // cout << "p10" << endl;
      // cout << "size:" << pff-> m_p10CqiRxed.size() << endl;
      // for(p10it = pff-> m_p10CqiRxed.begin();p10it != pff-> m_p10CqiRxed.end();p10it++)
      // {
      //   cout << p10it -> first << endl;
      //   cout << (u_int16_t)(p10it -> second) << endl;
      //   cout << "---------------" << endl;

      // }

      // std::map <uint16_t,SbMeasResult_s>::iterator a30it;
      // cout << "a30" << endl;
      // cout << "size:" << pff-> m_a30CqiRxed.size() << endl;
      // for(a30it = pff-> m_a30CqiRxed.begin();a30it != pff-> m_a30CqiRxed.end();a30it++)
      // {
      //   cout << a30it -> first << endl;
        
      //   cout << (u_int16_t)(a30it -> second.m_bwPart.m_cqi) << endl;
      //   if(a30it -> second.m_higherLayerSelected.begin()->m_sbCqi.size() > 0 )
      //   {
      //     cout << (u_int16_t)(*(a30it -> second.m_higherLayerSelected.begin()->m_sbCqi.begin()) )<< endl;
      //   }
      //   if(a30it -> second.m_ueSelected.m_sbCqi.size()>0){
      //     cout << (u_int16_t)(*(a30it -> second.m_ueSelected.m_sbCqi.begin()) )<< endl;
      //   }
      //   cout << "---------------" << endl;

      // }

      for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
               it = pff->m_rlcBufferReq.begin ();
           it != pff->m_rlcBufferReq.end (); it++)
        {
          ns3::EpsBearer::Qci qci;

           cout << i+1 << "-" << it->first.m_rnti  << ":"<< (uint16_t)(it->first.m_lcId) <<  endl;
            cout << "时延tx:---" << it->second.m_rlcTransmissionQueueHolDelay << "时延retx:---" << it->second.m_rlcRetransmissionHolDelay<< " tx:" << it->second.m_rlcTransmissionQueueSize << " retx:" << it->second.m_rlcRetransmissionQueueSize << endl;
              // temp.push_back ((u_int16_t)(pff-> m_p10CqiRxed.find(it->first.m_rnti) ->second));
          if(enbRrc->HasUeManager(it->first.m_rnti))
        {
          std::map <uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator pdrb = enbRrc->GetUeManager(it->first.m_rnti)->m_drbMap.begin();
          // cout << "===========" << endl;
          qci = (++pdrb)->second->m_epsBearer.qci;
          // cout << (++pdrb)->second->m_epsBearer.qci << endl;
          // cout << (uint16_t)(pdrb->first) << endl;
          
         }
        Ptr<LteAmc> amc = pff-> m_amc;
        uint8_t cqi = pff->m_p10CqiRxed.find(it->first.m_rnti)->second;
        uint8_t mcs = amc->GetMcsFromCqi(cqi);
        int nOfprb = 0;
        uint32_t trans = 0;
        if (it->first.m_rnti != 0 && it->second.m_rlcTransmissionQueueSize != 0)
        {
          trans = it->second.m_rlcTransmissionQueueSize;
          nOfprb = 2;
          while((uint32_t)amc->GetDlTbSizeFromMcs(mcs, nOfprb) / 8 < it->second.m_rlcTransmissionQueueSize && nOfprb <= 24)
          {
            nOfprb += 2;
          } 
        }
        
        
        for(int k = 0 ; k < nOfprb/2; k++)
        {

          list<int> temp;
          if (it->first.m_rnti != 0 && it->second.m_rlcTransmissionQueueSize != 0 && trans > 0)
            {
             
              if(trans - amc->GetDlTbSizeFromMcs(mcs, 2)/8 >= 0)
                temp.push_back (amc->GetDlTbSizeFromMcs(mcs, 2)/8);
              else
                temp.push_back (amc->GetDlTbSizeFromMcs(mcs, 2)/8);
             
              trans -= amc->GetDlTbSizeFromMcs(mcs, 2)/8;
           
              temp.push_back (i);
              temp.push_back (it->first.m_rnti);
              temp.push_back (cqi);
            
                if(enbRrc->HasUeManager(it->first.m_rnti))
                {
                  
                  
                temp.push_back (qci);
                
                }
                else
                {
             
                  temp.push_back (9);
                
                }
            }
          ls.push_back (temp);

        }
        }
      ls.sort (); //依等待传输的数据量排序
      // ls.reverse (); //上面是从小到大，故进行逆序
    }
  uint16_t num_ue = 0; //统计总的业务请求数
  for (list<list<int>>::iterator it = ls.begin (); it != ls.end () && num_ue < 1200; it++)
    {
      for (list<int>::iterator iz = it->begin (); iz != it->end (); iz++)
        {
          if (iz == it->begin ())
            continue;
          box->AddValue (*(iz)); //将请求按照(CellID1,RNTI1,CellID2,RNTI2,CellID3,RNTI3,……)的形式存在OpenGymBoxContainer中等待传送
        }
      num_ue++;
    }

  num_ue_ = num_ue;

  //如果请求数不足84，则进行补0
  if (ls.size () <= 1200)
    {
      for (uint16_t i = 0; i < 1200 - ls.size (); i++)
        {
          box->AddValue (0);
          box->AddValue (0);
          box->AddValue (0);
          box->AddValue (0);
        }
    }
  else
    {
    }

  // cout << "Observation:(排序后,每两个数一组代表(CellId,RNTI)) \n" << box << endl;

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

/*
Execute received actions
ns3接收到从gym中传递过来的封装在OpenGymDataContainer中的动作action，并将其以适当的形式作为参数传递到调度器中执行
*/
bool
MyExecuteActions (Ptr<OpenGymDataContainer> action)
{
  Ptr<OpenGymTupleContainer> tu_ac = DynamicCast<OpenGymTupleContainer> (
      action); //将传递过来的action自动转换为OpenGymTupleContainer
  //传递过来的OpenGymTupleContainer封装的动作的形式是(CellID1,RNTI1,资源编号1,CellID2,RNTI2,资源编号2,CellID3,RNTI3,资源编号3,……)
  //从OpenGymTupleContainer中读取数据并保存到字符串中
  string ac_s[7] = {"", "", "", "", "", "", ""};
  for (uint32_t i = 0; i < tu_ac->size(); i += 3)
    {
      //如果没有动作，标记为9999
      if (DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 2))->GetValue () == 9999)
        {
          ac_s[0] += to_string (9999);
          ac_s[1] += to_string (9999);
          ac_s[2] += to_string (9999);
          ac_s[3] += to_string (9999);
          ac_s[4] += to_string (9999);
          ac_s[5] += to_string (9999);
          ac_s[6] += to_string (9999);
        }
      else
        {
          //第i个波束（eNB）的调度信息保存为 RNTI1 资源编号1 RNTI2 资源编号2 RNTI3 资源编号3 …… 形式的字符串
          ac_s[DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i))->GetValue ()] +=
              (to_string (DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 1))->GetValue ()) +
               " " +
               to_string (DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 2))->GetValue ()));
          ac_s[DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i))->GetValue ()] += " ";
        }
    }

  NS_LOG_UNCOND ("Action: \n" << tu_ac);
  //以字符串的形式将动作传递到各个波束（eNB）进行调度
  for (uint16_t i = 0; i < 7; i++)
    {
      
      PointerValue ptr;
      enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetCcMap ().at (0)->GetAttribute (
          "FfMacScheduler", ptr); //获取波束(eNB)i的调度器
      Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
      // ac_s[i] = to_string(i) + " " + ac_s[i]; 
      // cout << "===" << ac_s[i].size() << endl;
      StringValue c;
      //通过SetAttribute将动作参数传到调度器中
      ff->SetAttribute ("test", StringValue (ac_s[i]));
      ff->SetAttribute ("cellid", StringValue (to_string(i)));
      //分别打印各个波束的调度信息
      ff->GetAttribute ("test", c);
      vector<string> vcs = split (c.Get ().c_str (), " ");
      // string res_alloc = "";

      // for (uint16_t j = 0; j < vcs.size (); j += 2)
      //   {
      //     res_alloc += vcs[j];
      //     res_alloc += ":";
      //     res_alloc += vcs[j + 1];
      //     res_alloc += " ";
      //   }
      // cout << "第" << i + 1 << "个波束分配情况"
          //  << "(RNTI:资源编号):" << endl;
      // cout << res_alloc << endl;
    }
  cout << "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+" << endl;
   for(uint32_t i = 0; i < ueDevs.GetN(); i++)
  {
    
    Ptr<LteSpectrumPhy> interf = ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetPhy()->GetDownlinkSpectrumPhy();
    cout << interf-> m_interferenceData-> m_lastChangeTime<< "-------------" << endl;
    cout << interf-> m_interferenceData->m_receiving << endl;
     if(interf-> m_interferenceData->m_receiving && (Simulator::Now () > interf-> m_interferenceData-> m_lastChangeTime))
    {
      SpectrumValue allSignals = *(interf-> m_interferenceData->m_allSignals);
      SpectrumValue rxSignal = *(interf-> m_interferenceData->m_rxSignal);
      SpectrumValue noise = *(interf-> m_interferenceData->m_noise);
      SpectrumValue interference = allSignals-rxSignal+noise;
      SpectrumValue sinr = rxSignal/interference;
      cout <<"sinr: " << sinr << endl;
    }

    // interf = ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetPhy()->GetDownlinkSpectrumPhy();
    // // if (interf-> m_interferenceData->m_receiving && (Simulator::Now () > interf-> m_interferenceData-> m_lastChangeTime))
    // // {
      
    //   cout <<"===============" << endl;
    //   SpectrumValue allSignals = *(interf-> m_interferenceData->m_allSignals);
    //   SpectrumValue rxSignal = *(interf-> m_interferenceData->m_rxSignal);
    //   SpectrumValue noise = *(interf-> m_interferenceData->m_noise);
    //   SpectrumValue interference = allSignals-rxSignal+noise;
    //   SpectrumValue sinr = rxSignal/interference;
    //   cout <<"===============" << endl;
    //   cout <<"sinr: " << sinr << endl;
    //   cout << "m_allSignals: " << *(interf-> m_interferenceData->m_allSignals)<< endl;
    //   cout << "m_rxSignal: " << *(interf-> m_interferenceData->m_rxSignal)<< endl;
    //   // cout << "m_noise: " << *(interf-> m_interferenceData->m_noise)<< endl;
    //   cout << "interference: " << (allSignals-rxSignal+noise) << endl;
    // // }
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



  // tracetest();
  vector< struct rParameters > rv(enbDevs.GetN());
 
  for(uint8_t i = 0; i < 7; i++)
  {
      PointerValue ptr;
      enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetCcMap ().at (0)->GetAttribute (
          "FfMacScheduler", ptr);

      Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
      Ptr<DacFfMacScheduler> pff = ff->GetObject<DacFfMacScheduler> ();

      
      enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetCcMap ().at (0)->GetAttribute (
          "FfMacScheduler", ptr);
     Ptr<LteEnbRrc> enbRrc = enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetRrc ();
      rParameters r;
      r.cellid = i;
      map<uint32_t, uint32_t> bitmap;
      for(vector <struct BuildDataListElement_s>::iterator it = pff->m_ret.m_buildDataList.begin(); it != pff->m_ret.m_buildDataList.end();it++)
      {
        bitmap.insert(std::pair <uint32_t, uint32_t> (it->m_dci.m_rnti, it->m_dci.m_rbBitmap));
      }
      r.peruerbbitmap = bitmap;         //t时刻调度结果



      
      map<uint32_t, vector<uint32_t>> ue_states;
      for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
               it = pff->m_rlcBufferReq.begin ();
           it != pff->m_rlcBufferReq.end (); it++)
        {
          ns3::EpsBearer::Qci qci;

          //  cout << i+1 << "-" << it->first.m_rnti  << ":"<< (uint16_t)(it->first.m_lcId) <<  endl;
          //   cout << "时延tx:---" << it->second.m_rlcTransmissionQueueHolDelay << "时延retx:---" << it->second.m_rlcRetransmissionHolDelay<< " tx:" << it->second.m_rlcTransmissionQueueSize << " retx:" << it->second.m_rlcRetransmissionQueueSize << endl;
              // temp.push_back ((u_int16_t)(pff-> m_p10CqiRxed.find(it->first.m_rnti) ->second));
          if(enbRrc->HasUeManager(it->first.m_rnti))
        {
          std::map <uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator pdrb = enbRrc->GetUeManager(it->first.m_rnti)->m_drbMap.begin();
          // cout << "===========" << endl;
          qci = (++pdrb)->second->m_epsBearer.qci;
          // cout << (++pdrb)->second->m_epsBearer.qci << endl;
          // cout << (uint16_t)(pdrb->first) << endl;
          
         }
          Ptr<LteAmc> amc = pff-> m_amc;
          uint8_t cqi = pff->m_p10CqiRxed.find(it->first.m_rnti)->second;
          vector<uint32_t> temp(2);
          temp[0] = cqi;
          temp[1] = (uint32_t)qci;
          ue_states.insert(std::pair <uint32_t, vector<uint32_t>>(it->first.m_rnti, temp));


          
        }
        r.uestates = ue_states;  //cqi和qci信息

        map<uint32_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters> rlcbuffer;
        // map<uint32_t, vector<uint32_t>> ue_states;
      for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
               it = pff->m_rlcBufferReq.begin ();
           it != pff->m_rlcBufferReq.end (); it++)
        {
          rlcbuffer.insert(pair<uint32_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>(it->first.m_rnti, it->second));
        }
        r.rlcbuffer = rlcbuffer;  //RLCBUFFER信息






        map<uint32_t, vector<double>> phy_rxstates;
        ifstream file;
        file.open ("/home/liqi/ns3-gym/ns3-gym-master/DlRxPhyStats.txt"); //打开系统生成的日志
        char szbuff[1024] = {0};
        string za;

        Time tim = Simulator::Now (); //获取系统当前时刻
      
        file.getline (szbuff, 1024);
        while (!file.eof ())
          {
            // tobn += 1;
            file.getline (szbuff, 1024);
            za = szbuff;
            vector<string> sp = split (za.c_str (), "\t");
            //获取系统上个时刻的日志信息，得到需要的数据
            if (sp.size () == 12 && stoi(sp[1].c_str()) == i+1)
              {
                if(phy_rxstates.find(stoi(sp[3].c_str())) == phy_rxstates.end())
                {
                  vector<double> temp(6,0);
                  //0 第一次发传输时间
                  //1 最后一次传输时间
                  //2 总传送块数
                  //3 正确传输的块数
                  //4 总传输量
                  //5 传对的量
                  
                  temp[0] = stoi(sp[0].c_str());
                  temp[1] = stoi(sp[0].c_str());
                  temp[2] += 1;
                  temp[4] += stoi(sp[7].c_str());
           
                  if(stoi(sp[10].c_str()) == 1)
                  {
                    temp[3] += 1;
                    temp[5] += stoi(sp[7].c_str());
                  }
                  phy_rxstates.insert(std::pair<uint32_t, vector<double>>(atoi(sp[3].c_str()), temp));

                }
                else
                {
                  // vector<double> temp = phy_rxstates.find(atoi(sp[3].c_str()))->second;
                  phy_rxstates.find(atoi(sp[3].c_str()))->second[1] = stoi(sp[0].c_str());;
                  phy_rxstates.find(atoi(sp[3].c_str()))->second[2] += 1;
                  phy_rxstates.find(atoi(sp[3].c_str()))->second[4] += stoi(sp[7].c_str());
                  if(stoi(sp[10].c_str()) == 1)
                  {
                    phy_rxstates.find(atoi(sp[3].c_str()))->second[3] += 1;
                    phy_rxstates.find(atoi(sp[3].c_str()))->second[5] += stoi(sp[7].c_str());
                  }
                  // phy_rxstates.insert(std::pair<uint32_t, vector<double>>(atoi(sp[3].c_str()), temp));
                }
                
              }
          }
          r.phyrxstates = phy_rxstates;   //物理层接收情况










        rv[i] = r;


  



  }
  if(stepCounter > 20)
  {
  for(uint32_t i = 0; i < ueDevs.GetN(); i++)
  {
    
    Ptr<LteSpectrumPhy> interf = ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetPhy()->GetDownlinkSpectrumPhy();
    cout << interf-> m_interferenceData-> m_lastChangeTime<< "-------------" << endl;
    cout << interf-> m_interferenceData->m_receiving << endl;
     if(interf-> m_interferenceData->m_receiving && (Simulator::Now () > interf-> m_interferenceData-> m_lastChangeTime))
    {
      SpectrumValue allSignals = *(interf-> m_interferenceData->m_allSignals);
      SpectrumValue rxSignal = *(interf-> m_interferenceData->m_rxSignal);
      SpectrumValue noise = *(interf-> m_interferenceData->m_noise);
      SpectrumValue interference = allSignals-rxSignal+noise;
      SpectrumValue sinr = rxSignal/interference;
      cout <<"sinr: " << sinr << endl;
    }

    interf = ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetPhy()->GetDownlinkSpectrumPhy();
    if (interf-> m_interferenceData->m_receiving && (Simulator::Now () > interf-> m_interferenceData-> m_lastChangeTime))
    {
      
      cout <<"===============" << endl;
      SpectrumValue allSignals = *(interf-> m_interferenceData->m_allSignals);
      SpectrumValue rxSignal = *(interf-> m_interferenceData->m_rxSignal);
      SpectrumValue noise = *(interf-> m_interferenceData->m_noise);
      SpectrumValue interference = allSignals-rxSignal+noise;
      SpectrumValue sinr = rxSignal/interference;
      cout <<"===============" << endl;
      cout <<"sinr: " << sinr << endl;
      cout << "m_allSignals: " << *(interf-> m_interferenceData->m_allSignals)<< endl;
      cout << "m_rxSignal: " << *(interf-> m_interferenceData->m_rxSignal)<< endl;
      // cout << "m_noise: " << *(interf-> m_interferenceData->m_noise)<< endl;
      cout << "interference: " << (allSignals-rxSignal+noise) << endl;
    }
  }
  }
  rewardParameters.push_back(rv);
  vector< struct rParameters > rps0;
  vector< struct rParameters > rps1;
  vector< struct rParameters > rps3;
  double reward1 = 0.0; //delay
  double reward2 = 0.0; //吞吐率
  double reward3 = 0.0; //负奖励
  if(rewardParameters.size() == 4)
  {
    rps0 = *(rewardParameters.begin());
    rps1 = *(++rewardParameters.begin());//
    rps3 = rewardParameters.back();
    
    for(uint32_t i = 0 ; i < enbDevs.GetN(); i++)
    {
      map<uint32_t, ns3::FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
      for(it = rps1[i].rlcbuffer.begin(); it != rps1[i].rlcbuffer.end(); it++)
      {
        if(rps1[i].uestates.find(it->first) != rps1[i].uestates.end())
        {
          uint32_t qci = rps1[i].uestates.find(it->first)->second[1];
          reward1 += 1.0 - (double)it->second.m_rlcTransmissionQueueHolDelay/delat_buget[qci-1];
        }
      }


      map<uint32_t, vector<double>>::iterator it1;
      for(it1 = rps3[i].phyrxstates.begin();it1 != rps3[i].phyrxstates.end(); it1++)
      {
        reward2 += it1->second[5]/(it1->second[4] + (double)rps0[i].rlcbuffer.find(it1->first)->second.m_rlcTransmissionQueueSize); //已传（传对）/(已传（全部）+待传)
      }
       
       
       
       
      map<uint32_t, ns3::FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it2;
      for(it2 = rps0[i].rlcbuffer.begin(); it2 != rps0[i].rlcbuffer.end(); it2++)
      {
        
        if(rps1[i].peruerbbitmap.find(it2->first) == rps1[i].peruerbbitmap.end() && (it2->second.m_rlcTransmissionQueueSize > 0 || it2->second.m_rlcRetransmissionQueueSize > 0))
        {
         
          uint32_t cqi = rps0[i].uestates.find(it2->first)->second[0];
          reward3 -= (double)cqi/15;
        }
      }




    }
    rewardParameters.erase(rewardParameters.begin());
  }
  
  // reward3 = reward2;
  // reward2 = reward1;
  // reward1 = 0.0;

  // // queue3 = queue2;
  // // queue2 = queue1;
  // // queue1 = list<list<double>>();
  

  // //delay的获取，rlcbuffer中的队首时延会在调度完成时立刻更新，需要有中间量进行存储到3个tti之后进行计算,或者在此刻计算存到奖励中间变量
  // for(uint8_t i = 0; i < 7; i++)
  // {
  //   cout << "ceiiid:" << i+1 << "*****************" << endl;
  //   PointerValue ptr;
  //     enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetCcMap ().at (0)->GetAttribute (
  //         "FfMacScheduler", ptr);

  //     Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
  //     Ptr<DacFfMacScheduler> pff = ff->GetObject<DacFfMacScheduler> ();

  //     for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
  //              it1 = pff->m_rlcBufferReq.begin ();
  //          it1 != pff->m_rlcBufferReq.end (); it1++)
  //     {
        
  //       bool find = false;
  //       for(vector <struct BuildDataListElement_s>::iterator it = pff->m_ret_p2.m_buildDataList.begin(); it != pff->m_ret_p2.m_buildDataList.end();it++)
        
  //       {
  //         if(it1->first.m_rnti == it->m_rnti){
  //           find = true;
  //           list<double> temp;
  //           //待传数据量的获取，
  //           temp.push_back(i);
  //           temp.push_back(it->m_rnti);
  //           temp.push_back(it1->second.m_rlcRetransmissionQueueSize);
  //            Ptr<LteEnbRrc> enbRrc = enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetRrc ();
  //            std::map <uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator pdrb = enbRrc->GetUeManager(it1->first.m_rnti)->m_drbMap.begin();
          
  //           uint16_t delaybuget = (++pdrb)->second->m_epsBearer.GetPacketDelayBudgetMs();
  //           reward1 += ((double)delaybuget - (double)it1->second.m_rlcRetransmissionHolDelay) / (double)delaybuget;
            

  //         }
  //         if(find) 
  //           break;
  //       }
  //       // if(!find)
  //       // {
  //       //   reward1 += -2;
  //       // }
  //       // cout <<"rnti: " << it->m_rnti << endl;
  //       // cout <<"m_rbBitmap: "<< it->m_dci.m_rbBitmap << endl;

  //     //     for(uint8_t k = 0 ; k < ueDevs.GetN(); k++)
  //     //   {
  //     //     // cout << "+++++++++++++++++++++++++++" << endl;
  //     //     // double r = 0;
  //     //     // double count = 0;
  //     //     Ptr<LteSpectrumPhy> interf = ueDevs.Get(k)->GetObject<LteUeNetDevice> ()->GetPhy()->GetDownlinkSpectrumPhy();

          
  //     //     // cout << "=========================" << endl;
  //     //     if(interf-> m_interferenceData->m_receiving && (Simulator::Now () > interf-> m_interferenceData-> m_lastChangeTime) && ueDevs.Get(k)->GetObject<LteUeNetDevice> ()->GetRrc()->GetCellId() == i+1 && ueDevs.Get(k)->GetObject<LteUeNetDevice> ()->GetRrc()->GetRnti() == it->m_rnti)
  //     //     {
  //     //       SpectrumValue allSignals = *(interf-> m_interferenceData->m_allSignals);
  //     //       SpectrumValue rxSignal = *(interf-> m_interferenceData->m_rxSignal);
  //     //       SpectrumValue noise = *(interf-> m_interferenceData->m_noise);
  //     //       SpectrumValue interference = allSignals-rxSignal+noise;
  //     //       SpectrumValue sinr = rxSignal/interference;
  //     //       uint32_t bitmap = it->m_dci.m_rbBitmap;
    

  //     //       // cout << "*****************" << endl;
  //     //       vector <int> m;
  //     //       uint32_t mask = 0x1;
    
  //     //       for (int j = 0; j < 32; j++)
  //     //         {
  //     //           if (((bitmap & mask) >> j) == 1)
  //     //             {
  //     //               m.push_back (j);
                  
  //     //             }
  //     //           mask = (mask << 1);
  //     //         }
  //     //         cout << "prbmap: ";
  //     //         for(uint8_t j =0; j < m.size(); j++)
  //     //         {
  //     //           cout << m[j] << " ";
                
  //     //         }
  //     //         cout << endl;
  //     //         cout << endl;
            
  //     //     }
  //     //     // cout << "///////////////////////////" << endl;
         
  //     // }

        
  


  //     }
  // }


// for(uint8_t i = 0; i < 7; i++)
//   {
//     cout << "ceiiid:" << i+1 << "*****************" << endl;
//     PointerValue ptr;
//       enbDevs.Get (i)->GetObject<LteEnbNetDevice> ()->GetCcMap ().at (0)->GetAttribute (
//           "FfMacScheduler", ptr);

//       Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
//       Ptr<PfFfMacScheduler> pff = ff->GetObject<DacFfMacScheduler> ();

      
//       for(vector <struct BuildDataListElement_s>::iterator it = pff->m_ret_p.m_buildDataList.begin(); it != pff->m_ret_p.m_buildDataList.end();it++)
//       {
        
//         cout <<"rnti: " << it->m_rnti << endl;
//         cout <<"m_rbBitmap: "<< it->m_dci.m_rbBitmap << endl;

//           for(uint8_t k = 0 ; k < ueDevs.GetN(); k++)
//         {
//           // cout << "+++++++++++++++++++++++++++" << endl;
//           // double r = 0;
//           // double count = 0;
//           Ptr<LteSpectrumPhy> interf = ueDevs.Get(k)->GetObject<LteUeNetDevice> ()->GetPhy()->GetDownlinkSpectrumPhy();

          
//           // cout << "=========================" << endl;
//           // if(interf-> m_interferenceData->m_receiving && (Simulator::Now () > interf-> m_interferenceData-> m_lastChangeTime) && ueDevs.Get(k)->GetObject<LteUeNetDevice> ()->GetRrc()->GetCellId() == i+1 && ueDevs.Get(k)->GetObject<LteUeNetDevice> ()->GetRrc()->GetRnti() == it->m_rnti)
//           if( ueDevs.Get(k)->GetObject<LteUeNetDevice> ()->GetRrc()->GetCellId() == i+1 && ueDevs.Get(k)->GetObject<LteUeNetDevice> ()->GetRrc()->GetRnti() == it->m_rnti)
//           {
//             SpectrumValue allSignals = *(interf-> m_interferenceData->m_allSignals);
//             SpectrumValue rxSignal = *(interf-> m_interferenceData->m_rxSignal);
//             SpectrumValue noise = *(interf-> m_interferenceData->m_noise);
//             SpectrumValue interference = allSignals-rxSignal+noise;
//             SpectrumValue sinr = rxSignal/interference;
//             uint32_t bitmap = it->m_dci.m_rbBitmap;
//             cout << allSignals << endl;
//             cout << rxSignal << endl;
//             cout << interference << endl;
//             cout << sinr << endl;

//             // cout << "*****************" << endl;
//             vector <int> m;
//             uint32_t mask = 0x1;
    
//             for (int j = 0; j < 32; j++)
//               {
//                 if (((bitmap & mask) >> j) == 1)
//                   {
//                     m.push_back (j);
                  
//                   }
//                 mask = (mask << 1);
//               }
//               cout << "prbmap: ";
//               for(uint8_t j =0; j < m.size(); j++)
//               {
//                 cout << m[j] << " ";
                
//               }
//               cout << endl;
//               cout << endl;
            
//           }
//           // cout << "///////////////////////////" << endl;
         
//       }

        
  


//       }
//   }

        






      
       
      
 

  
    // }




 






  // double reward = 0.0;
  //  monitor->CheckForLostPackets (); 
  // // double Throughput = 0.0;
  // Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  // map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  
  // for (map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  //     {
  //     Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
  //       if(t.sourceAddress == Ipv4Address("1.0.0.2"))
  //       {
          
  //         // //   reward +=(-(float)(i->second.lostPackets)/i->second.txPackets);
          
  //         // cout << "Flow ID: " << i->first << " Src Addr " <<  t.sourceAddress << " Dst Addr " << t.destinationAddress << endl;
  //         // cout << "Tx Packets = " << i->second.txPackets << endl;
  //         // cout << "Rx Packets = " << i->second.rxPackets << endl;
  //         // cout << "Lost Packets = " << i->second.lostPackets << endl;
  //         // cout << "delay = " << i->second.lastDelay.GetMilliSeconds() << endl;
  //         // // reward += (-(float)(i->second.lastDelay.GetMilliSeconds())/500);
  //         reward += i->second.rxBytes * 8.0 /(i->second.timeLastRxPacket.GetSeconds()-i->second.timeFirstTxPacket.GetSeconds())/ 1024;
  //       }
  //     }
      
  //     // if(n > 0)
  //     //   reward = reward/(n);
     
  //   // reward = C;

  cout << "reward1: " << reward1 << endl;
  cout << "reward2: " << reward2 << endl;
  cout << "reward3: " << reward3 << endl;
  return 0.3*reward1+0.65*reward2+0.05*reward3;
  
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
  uint32_t simSeed = 15;
  double simulationTime = 50; //seconds
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

  uint16_t numberOfRandomUes = 1;
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
  lteHelper->SetSchedulerType("ns3::DacFfMacScheduler");
  
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.0)));
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
  enbPositionAlloc->Add (Vector (0.0, 0.0, radius)); // eNB1 (0,0,0)
  enbPositionAlloc->Add (Vector (-1.732 / 2.0 * radius, 1.5 * radius, radius)); // eNB2
  enbPositionAlloc->Add (Vector (1.732 / 2.0 * radius, 1.5 * radius, radius)); // eNB3
  enbPositionAlloc->Add (Vector (1.732 * radius, 0.0, radius)); //eNB4
  enbPositionAlloc->Add (Vector (1.732 / 2.0 * radius, -1.5 * radius, radius)); //eNB5
  enbPositionAlloc->Add (Vector (-1.732 / 2.0 * radius, -1.5 * radius, radius)); //eNB6
  enbPositionAlloc->Add (Vector (-1.732 * radius, 0.0, radius)); //eNB7
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (enbPositionAlloc);
  mobility.Install (enbNodes);

  Ptr<RandomBoxPositionAllocator> randomUePositionAlloc =
      CreateObject<RandomBoxPositionAllocator> ();
  Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable> ();
  xVal->SetAttribute ("Min", DoubleValue (-15000.0));
  xVal->SetAttribute ("Max", DoubleValue (15000.0));
  randomUePositionAlloc->SetAttribute ("X", PointerValue (xVal));
  Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable> ();
  yVal->SetAttribute ("Min", DoubleValue (-15000.0));
  yVal->SetAttribute ("Max", DoubleValue (15000.0));
  randomUePositionAlloc->SetAttribute ("Y", PointerValue (yVal));
  Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable> ();
  zVal->SetAttribute ("Min", DoubleValue (0));
  zVal->SetAttribute ("Max", DoubleValue (1));
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
  startTimeSeconds->SetAttribute ("Max", DoubleValue (10));
  Ptr<UniformRandomVariable> stopTimeSeconds = CreateObject<UniformRandomVariable> ();
  stopTimeSeconds->SetAttribute ("Min", DoubleValue (20));
  stopTimeSeconds->SetAttribute ("Max", DoubleValue (50));

  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

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


          // UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
          // dlClientHelper.SetAttribute ("MaxPackets", UintegerValue (1000000));
          // dlClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (1)));



          OnOffHelper dlClientHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (ueIpIfaces.GetAddress (u), dlPort));
          cout << "ue-" << u <<  ": " << ueIpIfaces.GetAddress (u) << endl;
          dlClientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));   
          dlClientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.1]"));
          dlClientHelper.SetAttribute("PacketSize", UintegerValue (256));
          dlClientHelper.SetAttribute("DataRate", DataRateValue (DataRate ("100kb/s")));
          dlClientHelper.SetAttribute("MaxBytes", UintegerValue (1024000));
          // dlClientHelper.SetAttribute ("MaxPackets", UintegerValue (100));
          // dlClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (20.0)));
          clientApps.Add (dlClientHelper.Install (remoteHost));
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), dlPort));
                               
          serverApps.Add (dlPacketSinkHelper.Install (ue));


          

          OnOffHelper ulClientHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (remoteHostAddr, ulPort));
          ulClientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.4]"));   
          ulClientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.1]"));
          


          // UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
          // ulClientHelper.SetAttribute ("MaxPackets", UintegerValue (1000000));
          // ulClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (1)));
          // ulClientHelper.SetAttribute ("MaxPackets", UintegerValue (100));
          // ulClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (5.0)));
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
              lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), EpsBearer(EpsBearer::GBR_GAMING), tft);
            }

          // Time startTime = Seconds (startTimeSeconds->GetValue ());
          // Time stopTime = Seconds (stopTimeSeconds->GetValue ());
 
        
          // cout << "===" << u << endl;
          
        }
        // cout << serverApps.GetN() << endl;
         Time startTime = Seconds (startTimeSeconds->GetValue ());
          Time stopTime = Seconds (stopTimeSeconds->GetValue ());
          cout << startTime << endl;
          // serverApps.Get(2*u)->SetStartTime(startTime);
          // serverApps.Get(2*u+1)->SetStartTime(startTime);
          // clientApps.Get(2*u)->SetStartTime (startTime);
          // clientApps.Get(2*u+1)->SetStartTime (startTime);

          // serverApps.Get(2*u)->SetStopTime (startTime+stopTime );
          // serverApps.Get(2*u+1)->SetStopTime (startTime+stopTime );
          // clientApps.Get(2*u)->SetStopTime (startTime+stopTime );
          // clientApps.Get(2*u+1)->SetStopTime (startTime+stopTime );
          serverApps.Get(2*u)->SetStartTime(Seconds(0.001));
          serverApps.Get(2*u+1)->SetStartTime(Seconds(0.001));
          clientApps.Get(2*u)->SetStartTime (Seconds(0.001));
          clientApps.Get(2*u+1)->SetStartTime (Seconds(0.001));

          serverApps.Get(2*u)->SetStopTime (Seconds(0.001)+stopTime );
          serverApps.Get(2*u+1)->SetStopTime (Seconds(0.001)+stopTime );
          clientApps.Get(2*u)->SetStopTime (Seconds(0.001)+stopTime );
          clientApps.Get(2*u+1)->SetStopTime (Seconds(0.001)+stopTime );
          
          

    }
    monitor = flowmon.Install(ueNodes);
  // monitor = flowmon.Install(enbNodes);
  // monitor = flowmon.Install(remoteHostContainer);
  // monitor = flowmon.Install(pgw);
  
  flowmon.InstallAll ();
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

  NS_LOG_UNCOND ("Simulation stop");

  openGym->NotifySimulationEnd ();
  Simulator::Destroy ();
  return 0;
}
