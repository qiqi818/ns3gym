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
#include "ns3/netanim-module.h"
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

struct rParameters {
  //map的key都为rnti
  uint32_t cellid;
  map<uint32_t, uint32_t> peruerbbitmap;  //调度情况
  map<uint32_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters> rlcbuffer;//rlc层初始情况   队首时延和待传数据量
  map<uint32_t, vector<double>> phyrxstates;//物理层接收状况统计
  map<uint32_t, double> rlcrxstate;//调度完成后rlc层情况   队首时延和剩余待传数据量
  map<uint32_t, vector<uint32_t>> uestates;  //ue的相关信息  现有cqi和qci
  map<uint32_t, SpectrumValue> sinr;   //信干噪比
};

list< vector< struct rParameters > > rewardParameters;

uint8_t  bandwidth = 25;
int      num_ue_ = 0;
uint16_t numberOfenb = 2;//enb数目
uint16_t numberOfRandomUes = 2;//ue数量
uint16_t numOfrbg = 12;
Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
FlowMonitorHelper flowmon;
Ptr<FlowMonitor> monitor;
NetDeviceContainer enbDevs;
NetDeviceContainer ueDevs;
Ptr<OpenGymInterface> openGym;
NS_LOG_COMPONENT_DEFINE ("OpenGym");

/*
split函数，分割字符串
 */
std::vector<std::string>
split (const char *s, const char *delim)
{
  std::vector<std::string> result;
  if (s && strlen (s)) {
    int len = strlen (s);
    char *src = new char[len + 1];
    strcpy (src, s);
    src[len] = '\0';
    char *tokenptr = strtok (src, delim);
    while (tokenptr != NULL) {
      std::string tk = tokenptr;
      result.emplace_back (tk);
      tokenptr = strtok (NULL, delim);
    }
    delete[] src;
  }
  return result;
}
/*
  获取物理层接收状态的回调函数
*/
void DlPhyReceptionCallback(std::string path, PhyReceptionStatParameters params)
{
    map<uint32_t, vector<double>>::iterator it = rewardParameters.back()[params.m_cellId-1].phyrxstates.find(params.m_rnti);
    if(it == rewardParameters.back()[params.m_cellId-1].phyrxstates.end() || (it != rewardParameters.back()[params.m_cellId-1].phyrxstates.end() && (int64_t)it->second[1] != params.m_timestamp))
    {
      
      if(it == rewardParameters.back()[params.m_cellId-1].phyrxstates.end())
      {
            // 0 第一次发传输时间
            // 1 最后一次传输时间
            // 2 总传送块数
            // 3 正确传输的块数
            // 4 总传输量
            // 5 传对的量
            // 6 最近一次发包是否正确
        vector<double> temp(7,0);
        temp[0] = params.m_timestamp;
        temp[1] = params.m_timestamp;
        temp[2] = 1;
        temp[4] = params.m_size;
        if(params.m_correctness == 1)
        {
          temp[3] = 1;
          temp[5] = params.m_size;
          temp[6] = 1;
        }
        else
        {
          temp[3] = 0;
          temp[5] = 0;
          temp[6] = 0;
        }
        rewardParameters.back()[params.m_cellId-1].phyrxstates.insert(pair<uint32_t, vector<double>>(params.m_rnti, temp));
      }
      else
      {
        it->second[1] = params.m_timestamp;
        it->second[2] += 1;
        it->second[4] += params.m_size;
        if(params.m_correctness == 1)
        {
          it->second[3] += 1;
          it->second[5] += params.m_size;
          it->second[6] = 1;
        }
        else
        {
          it->second[3] += 0;
          it->second[5] += 0;
          it->second[6] = 0;
        }
      }
    }
}

void tracephyrx()
{
  Config::Connect("/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/LteUePhy/DlSpectrumPhy/DlPhyReception",
                   MakeCallback(&DlPhyReceptionCallback));
}

Ptr<LteEnbNetDevice>
GetEnb (int enb_idx)
{
  return enbDevs.Get (enb_idx)->GetObject<LteEnbNetDevice> ();
}

Ptr<AsyncFfMacScheduler>
GetSchedulerForEnb (int enb_idx)
{
  PointerValue ptr;
  GetEnb (enb_idx)->GetCcMap ().at (0)->GetAttribute ("FfMacScheduler", ptr);
  Ptr<FfMacScheduler> ff = ptr.Get<FfMacScheduler> ();
  Ptr<AsyncFfMacScheduler> pff = ff->GetObject<AsyncFfMacScheduler> ();
  return pff;
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
  for (uint16_t i = 0; i < ueDevs.GetN()*numOfrbg; i++)
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

void PrintRlcStat()
{
  cout << "eNB\tRNTI\tLCID\t时延tx\tTXLen\t时延retx\tReTxLen" << endl;
  cout << "===============================================================" << endl;
  for (uint16_t i = 0; i < enbDevs.GetN(); i++)
  {
    Ptr<AsyncFfMacScheduler> pff = GetSchedulerForEnb(i);
    for (auto it = pff->m_rlcBufferReq.begin (); it != pff->m_rlcBufferReq.end (); it++)
    {
      uint16_t rnti = it->first.m_rnti;
      uint8_t lcid = it->first.m_lcId;
      FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& rrp = it->second;

      cout << (i + 1) << "\t" << rnti << "\t" << (uint16_t)lcid << "\t";
      cout << rrp.m_rlcTransmissionQueueHolDelay << "\t" << rrp.m_rlcTransmissionQueueSize << "\t";
      cout << rrp.m_rlcRetransmissionHolDelay << "\t" << rrp.m_rlcRetransmissionQueueSize << endl;
    }
  }
}

/*
Collect observations
用于从ns3中统计环境信息，封装到OpenGymBoxContainer中
*/
Ptr<OpenGymDataContainer>
MyGetObservation(void)
{
  stepCounter += 1;
  cout << "----------------------以上为step: " << stepCounter-1 << "时刻----------------------" << endl;
  cout << endl;
  cout << "----------------------以下为step: " << stepCounter << "时刻----------------------" << endl;
  cout << "OBS for step: " << stepCounter << " 调度前的RlcBuffer  " << Simulator::Now ().GetMilliSeconds() << endl;
  uint32_t nodeNum = enbDevs.GetN ();
  std::vector<uint32_t> shape = {
      nodeNum,
  };

  Ptr<OpenGymBoxContainer<uint32_t>> box = 
  CreateObject<OpenGymBoxContainer<uint32_t>>(shape); //创建OpenGymBoxContainer用以包装存储observation，进而传递给gym

  list<list<int>> ls;
  cout << "eNB\tRNTI\tLCID\tQCI\tCQI\t时延tx\tTXLen\t时延retx\tReTxLen" << endl;
  cout << "===============================================================" << endl;
  for (uint16_t i = 0; i < enbDevs.GetN(); i++)
  {
    Ptr<LteEnbRrc> enbRrc = GetEnb(i)->GetRrc ();
    Ptr<AsyncFfMacScheduler> pff = GetSchedulerForEnb(i);

    // std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
    for (auto it = pff->m_rlcBufferReq.begin (); it != pff->m_rlcBufferReq.end (); it++)
    {
      uint16_t rnti = it->first.m_rnti;
      uint8_t lcid = it->first.m_lcId;
      FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& rrp = it->second;

      ns3::EpsBearer::Qci qci = ns3::EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
      if (enbRrc->HasUeManager (rnti)) {
        // key <== drbID
        std::map<uint8_t, Ptr<LteDataRadioBearerInfo>> dm = enbRrc->GetUeManager(rnti)->m_drbMap;
        for(auto itt = dm.begin(); itt != dm.end(); itt++) {
          if(lcid == itt->second->m_logicalChannelIdentity) {
            qci = itt->second->m_epsBearer.qci;
            break;
          }
        }
      }
      uint8_t cqi = pff->m_p10CqiRxed.find (rnti)->second;
      cout << (i + 1) << "\t" << rnti << "\t" << (uint16_t) lcid << "\t";
      cout << (uint16_t) qci << "\t" << (uint16_t)cqi << "\t";
      cout << rrp.m_rlcTransmissionQueueHolDelay << "\t" << rrp.m_rlcTransmissionQueueSize << "\t";
      cout << rrp.m_rlcRetransmissionHolDelay << "\t" << rrp.m_rlcRetransmissionQueueSize << endl;

      Ptr<LteAmc> amc = pff->m_amc;
      uint8_t mcs = amc->GetMcsFromCqi (cqi);
      int nOfprb = 0;
      uint32_t trans = 0;
      if (rnti != 0 && rrp.m_rlcTransmissionQueueSize != 0)
      {
        trans = rrp.m_rlcTransmissionQueueSize;
        nOfprb = 2;
        //统计每个ue需要多少个prb
        while ((uint32_t) amc->GetDlTbSizeFromMcs (mcs, nOfprb) / 8 < rrp.m_rlcTransmissionQueueSize && nOfprb <= 24)
        {
          nOfprb += 2;
        }
      }

      //拆分状态
      for (int k = 0; k < nOfprb / 2; k++)
      {
        list<int> temp;
        if (rnti != 0 && rrp.m_rlcTransmissionQueueSize != 0 && trans > 0)
        {
          if (trans - amc->GetDlTbSizeFromMcs (mcs, 2) / 8 >= 0)
            temp.push_back (amc->GetDlTbSizeFromMcs (mcs, 2) / 8);
          else
            temp.push_back (amc->GetDlTbSizeFromMcs (mcs, 2) / 8);

          trans -= amc->GetDlTbSizeFromMcs (mcs, 2) / 8;

          temp.push_back (i);
          temp.push_back (rnti);
          temp.push_back (cqi);
          temp.push_back (qci);
        }
        ls.push_back (temp);
      }
    }
    ls.sort (); //依等待传输的数据量排序
  }
  uint16_t num_ue = 0; //统计总的业务请求数
  for (list<list<int>>::iterator it = ls.begin (); it != ls.end () && num_ue < enbDevs.GetN()*numOfrbg; it++)
  {
    for (list<int>::iterator iz = it->begin (); iz != it->end (); iz++)
    {
      if (iz == it->begin ())
        continue;
      box->AddValue (*(iz)); //将请求存在OpenGymBoxContainer中等待传送
    }
    num_ue++;
  }

  num_ue_ = num_ue;

  //进行补0
  if (ls.size () <= enbDevs.GetN()*numOfrbg)
  {
    for (uint16_t i = 0; i < enbDevs.GetN()*numOfrbg - ls.size (); i++)
      {
        box->AddValue (0);
        box->AddValue (0);
        box->AddValue (0);
        box->AddValue (0);
      }
  }
  else
  {
    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << ls.size() << endl;
  }
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
  Ptr<OpenGymTupleContainer> tu_ac = DynamicCast<OpenGymTupleContainer>(action); 
  //将传递过来的action自动转换为OpenGymTupleContainer
  //传递过来的OpenGymTupleContainer封装的动作的形式是(CellID1,RNTI1,资源编号1,CellID2,RNTI2,资源编号2,CellID3,RNTI3,资源编号3,……)
  //从OpenGymTupleContainer中读取数据并保存到字符串中
  string ac_s[enbDevs.GetN()];
  for(uint32_t j = 0 ; j < enbDevs.GetN(); j++)
  {
    ac_s[j] = "";
  }
  for (uint32_t i = 0; i < tu_ac->size(); i += 3) {
    uint32_t cell = DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 0))->GetValue ();
    uint32_t rnti = DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 1))->GetValue ();
    uint32_t rbg  = DynamicCast<OpenGymDiscreteContainer> (tu_ac->Get (i + 2))->GetValue ();

    //如果没有动作，标记为9999
    if(rbg == 9999) {
      for (uint32_t k = 0; k < enbDevs.GetN (); k++) {
        ac_s[k] += to_string (9999);
      }
    }
    else {
      //第i个波束（eNB）的调度信息保存为 RNTI1 资源编号1 RNTI2 资源编号2 RNTI3 资源编号3 …… 形式的字符串
      ac_s[cell] += (to_string(rnti) + " " + to_string(rbg));
      ac_s[cell] += " "; 
    }
  }
  cout << endl;
  NS_LOG_UNCOND ("(" << stepCounter << "时刻)Action: " << tu_ac << endl);
  //以字符串的形式将动作传递到各个波束（eNB）进行调度
  cout << endl;
  cout << "(" << stepCounter << "时刻)调度结果: " << endl;
  for (uint16_t i = 0; i < enbDevs.GetN(); i++) {
    Ptr<FfMacScheduler> ff = GetSchedulerForEnb(i);
    StringValue c;
    //通过SetAttribute将动作参数传到调度器中
    ff->SetAttribute ("test",   StringValue (ac_s[i]));
    ff->SetAttribute ("cellid", StringValue (to_string(i)));
    //分别打印各个波束的调度信息
    ff->GetAttribute ("test", c);
    // vector<string> vcs = split (c.Get ().c_str (), " ");
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
  /* 
    计算 T时刻 的reward所需的参数获取时间
    rlcbuffer                  T                调度器
    rlcrxstate                 T                调度器 使用schedule函数进行事件调度
    prbbitmap(调度结果)         T+1               调度器
    phyrxstate                 T+2              trace
    sinr                       T+2              使用schedule函数进行事件调度
  */
  vector< struct rParameters > rv(enbDevs.GetN());
 
  for(uint8_t i = 0; i < enbDevs.GetN(); i++)
  {
      Ptr<AsyncFfMacScheduler> pff = GetSchedulerForEnb(i);
      Ptr<LteEnbRrc> enbRrc = GetEnb(i)->GetRrc ();
      rParameters r;
      r.cellid = i;
      map<uint32_t, uint32_t> bitmap;
      for(vector <struct BuildDataListElement_s>::iterator it = pff->m_ret.m_buildDataList.begin(); it != pff->m_ret.m_buildDataList.end();it++)
      {
        // cout << i+1 << endl;
        // cout << "rnti: " << it->m_dci.m_rnti << endl;
        // cout << "rbbit: " << it->m_dci.m_rbBitmap << endl;
        bitmap.insert(std::pair <uint32_t, uint32_t> (it->m_dci.m_rnti, it->m_dci.m_rbBitmap));
      }
      r.peruerbbitmap = bitmap;         //t时刻调度结果

      map<uint32_t, vector<uint32_t>> ue_states;
      for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
               it = pff->m_rlcBufferReq.begin ();
           it != pff->m_rlcBufferReq.end (); it++)
        {
          ns3::EpsBearer::Qci qci;
          if(enbRrc->HasUeManager(it->first.m_rnti))
        {
          std::map <uint8_t, Ptr<LteDataRadioBearerInfo> >::iterator pdrb = enbRrc->GetUeManager(it->first.m_rnti)->m_drbMap.begin();
          qci = (++pdrb)->second->m_epsBearer.qci;
          
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
      for (std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
               it = pff->m_rlcBufferReq.begin ();
           it != pff->m_rlcBufferReq.end (); it++)
        {
          rlcbuffer.insert(pair<uint32_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>(it->first.m_rnti, it->second));
        }
        r.rlcbuffer = rlcbuffer;  //RLCBUFFER信息





        rv[i] = r;
        
  }
  

  rewardParameters.push_back(rv);
  //若此TTi没有接收数据，则不会触发tracephyrx，保存上一个TTi的phyrxstates
  if(rewardParameters.size() > 1)
  {
    for(uint32_t i = 0; i < enbDevs.GetN(); i++)
    {
      rewardParameters.back()[i].phyrxstates = (*(--(--rewardParameters.end())))[i].phyrxstates;
    }
  }


  //用收集的参数计算奖励----------------
  double reward = 0.0;
  vector< struct rParameters > rps0;
  vector< struct rParameters > rps1;
  vector< struct rParameters > rps2;
  vector< struct rParameters > rps3;
  double reward1 = 0.0; //delay
  double reward2 = 0.0; //信干噪比
  double reward3 = 0.0; //负奖励
  double reward4 = 0.0; //吞吐率
  // double unum = 0;
  // double sum = 0.0;
  // double count = 0.0;
  if(rewardParameters.size() == 4)
  {
    rps0 = *(rewardParameters.begin());//T
    rps1 = *(++rewardParameters.begin());//T+1
    rps2 = *(++(++rewardParameters.begin()));//T+2
    rps3 = rewardParameters.back();//T+3


    double tx_right = 0;//正确传输量
    double tx_total = 0;//全部待传量
    double tx = 0.0;//全部传输量（包含错传）

    for(uint32_t i = 0 ; i < enbDevs.GetN(); i++)
    {
      // map<uint32_t, ns3::FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
      // for(it = rps1[i].rlcbuffer.begin(); it != rps1[i].rlcbuffer.end(); it++)
      // {
      //   if(rps1[i].uestates.find(it->first) != rps1[i].uestates.end())
      //   {
      //     uint32_t qci = rps1[i].uestates.find(it->first)->second[1];
      //     reward1 += 1.0 - (double)it->second.m_rlcTransmissionQueueHolDelay/delat_buget[qci-1];
      //     unum += 1;
      //   }
      // }


      // map<uint32_t, ns3::SpectrumValue>::iterator it1;
      // for(it1 = rps2[i].sinr.begin();it1 != rps2[i].sinr.end(); it1++)
      // {
        
      //   for(uint32_t i = 0 ; i < 25; i++)
      //   {
      //     if(it1->second[i] > 0)
      //     {
      //       sum += it1->second[i];
      //       count += 1;
      //     }
      //   }
        
      // }
      
       
       
       
      // map<uint32_t, ns3::FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it2;
      // for(it2 = rps0[i].rlcbuffer.begin(); it2 != rps0[i].rlcbuffer.end(); it2++)
      // {
        
      //   if(rps1[i].peruerbbitmap.find(it2->first) == rps1[i].peruerbbitmap.end() && (it2->second.m_rlcTransmissionQueueSize > 0 || it2->second.m_rlcRetransmissionQueueSize > 0))
      //   {
         
      //     uint32_t cqi = rps0[i].uestates.find(it2->first)->second[0];
      //     reward3 -= (double)cqi/15;
      //   }
      // }

      map<uint32_t, ns3::FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it3;

      for(it3 = rps0[i].rlcbuffer.begin(); it3 != rps0[i].rlcbuffer.end(); it3++)
      {

        tx_total += it3->second.m_rlcTransmissionQueueSize;

        if(rps2[i].phyrxstates.find(it3->first) != rps2[i].phyrxstates.end())
        {
          if((uint32_t)rps2[i].phyrxstates.find(it3->first)->second[6] == 1 && (int64_t)rps2[i].phyrxstates.find(it3->first)->second[1] == Simulator::Now().GetMilliSeconds()-1)
          {

            tx_right += (rps0[i].rlcrxstate.find(it3->first)->second);
            
          }
          if((int64_t)rps2[i].phyrxstates.find(it3->first)->second[1] == Simulator::Now().GetMilliSeconds()-1)
          {

            tx += (rps0[i].rlcrxstate.find(it3->first)->second);
            
          }
          
        }

      }

    }
    // if(count > 0)
    //     reward2 += sum/count;
    if(tx_total > 0)
    {
      reward4 = tx_right/tx_total;
      cout << endl;
      cout << "(" << stepCounter-3 << "时刻)tx_right: " << tx_right << endl;
      cout << "(" << stepCounter-3 << "时刻)tx_total: " << tx_total << endl; 
    }  
    else
    {
      reward4 = 0;
    }
      
    rewardParameters.erase(rewardParameters.begin());
  }
  cout << "(" << stepCounter-3 << "时刻)reward: " << reward << endl;
  cout << "(" << stepCounter-3 << "时刻)reward1: " << reward1 << endl;
  cout << "(" << stepCounter-3 << "时刻)reward2: " << reward2 << endl;
  cout << "(" << stepCounter-3 << "时刻)reward3: " << reward3 << endl;
  cout << "(" << stepCounter-3 << "时刻)reward4: " << reward4 << endl;

  reward = reward2;


  return reward4-0.5;//加一个baseline 使吞吐率低于0.5为负奖励
  
}

//获取完成调度后的rlcbuffer信息
void rlc()
{
  cout << endl << "(" << stepCounter << "时刻)调度后的rlcbuffer: " << endl;
  cout << "eNB\tRNTI\tLCID\t时延tx\tTXLen\t时延retx\tReTxLen" << endl;
  cout << "===============================================================" << endl;
  for (uint32_t i = 0; i < enbDevs.GetN (); i++)
  {
    Ptr<LteEnbRrc> enbRrc = GetEnb(i)->GetRrc ();
    Ptr<AsyncFfMacScheduler> pff = GetSchedulerForEnb(i);
    for (auto it = pff->m_rlcBufferReq.begin (); it != pff->m_rlcBufferReq.end (); it++)
    {
      uint16_t rnti = it->first.m_rnti;
      uint8_t lcid = it->first.m_lcId;
      FfMacSchedSapProvider::SchedDlRlcBufferReqParameters &rrp = it->second;

      cout << (i + 1) << "\t" << rnti << "\t" << (uint16_t) lcid << "\t";
      cout << rrp.m_rlcTransmissionQueueHolDelay << "\t" << rrp.m_rlcTransmissionQueueSize << "\t";
      cout << rrp.m_rlcRetransmissionHolDelay << "\t" << rrp.m_rlcRetransmissionQueueSize << endl;
      if(rewardParameters.back()[i].rlcbuffer.find(rnti) != rewardParameters.back()[i].rlcbuffer.end() && 
         rewardParameters.back()[i].rlcbuffer.find(rnti)->second.m_rlcTransmissionQueueSize > 0)
      {
        double tx = (double)rewardParameters.back()[i].rlcbuffer.find(rnti)->second.m_rlcTransmissionQueueSize - rrp.m_rlcTransmissionQueueSize;
        // cout << "-----------tx: "<< tx << endl;
        rewardParameters.back()[i].rlcrxstate.insert(pair<uint32_t, double>(rnti, tx));
      }
    }
  }
}
//获取sinr
void sinr()
{
  cout << endl;
  cout <<"(" << stepCounter-2 << "时刻的)sinr: " << endl;
  //获取sinr
  for(uint32_t k = 0; k < enbDevs.GetN(); k++)
  {
    map<uint32_t, SpectrumValue> sinr;
    for(uint32_t i = 0; i < ueDevs.GetN(); i++)
    {
      Ptr<LteSpectrumPhy> interf = ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetPhy()->GetDownlinkSpectrumPhy();

      uint32_t rnti =  ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetRnti();
      uint32_t cellid = ueDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetCellId();
      if(interf-> m_interferenceData->m_receiving && (Simulator::Now () > interf-> m_interferenceData-> m_lastChangeTime) && cellid == k+1)
      {
        cout << "cellid: " << cellid << endl;
        cout << "rnti: " << rnti << endl;
        SpectrumValue allSignals = *(interf-> m_interferenceData->m_allSignals);
        SpectrumValue rxSignal = *(interf-> m_interferenceData->m_rxSignal);
        SpectrumValue noise = *(interf-> m_interferenceData->m_noise);
        SpectrumValue interference = allSignals-rxSignal+noise;
        SpectrumValue sinr_ = rxSignal/interference;
        list<std::vector<rParameters>>::iterator it = ++rewardParameters.begin();
        uint32_t bitmap = (*it)[k].peruerbbitmap.find(rnti)->second;
        uint32_t mask = 1;
        for(uint32_t x = 0; x < bandwidth/2; x++)
        {
          if(((mask<<x) & bitmap) == 0)
          {
            sinr_[2*x] = 0;
            sinr_[2*x+1] = 0;
          }
        }
        cout << "sinr: " << sinr_ << endl;
        sinr.insert(pair<uint32_t, SpectrumValue>(ueDevs.Get(i)->GetObject<LteUeNetDevice>()->GetRrc()->GetRnti(), sinr_));
      }

    }
    rewardParameters.back()[k].sinr = sinr;
  }
}
/*
ns3中的类似gym中的step函数
 */
// void
// ScheduleNextStateRead (double envStepTime, Ptr<OpenGymInterface> openGym)
// {
//   Simulator::Schedule (NanoSeconds (1), &rlc);//通过Schedule事件调度机制获取rlcbuffer
//   Simulator::Schedule (NanoSeconds (999999), &sinr);//通过Schedule事件调度机制在每个tti结束时获取sinr
//   Simulator::Schedule (Seconds(envStepTime), &ScheduleNextStateRead, envStepTime, openGym);
//   openGym->NotifyCurrentState ();
// }

struct UeReqItem {
  uint16_t cell_id;
  FfMacSchedSapProvider::SchedDlRlcBufferReqParameters req;
};

static std::set<uint16_t> g_criedEnb;
static std::vector<struct UeReqItem> g_arrReqs;
static void OnSchedDlNewTx(uint16_t enb_id, const std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>& req)
{
  cout << "@@@@@@@@@@@@@@@@@@@@@@@ OnSchedDlNewTx " << enb_id << endl;
  if(g_criedEnb.count(enb_id) > 0)  {
    cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!should not happen, Something wrong!!\n";
    return;
  }
  g_criedEnb.insert(enb_id);
  // std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator
  for (auto it = req.begin (); it != req.end (); it++)
  {
    struct UeReqItem uri;
    uri.cell_id = enb_id;
    uri.req = it->second;
    g_arrReqs.push_back(uri);
  }

  if(g_criedEnb.size() == enbDevs.GetN()) {
    // all the enb claimed their request
    Simulator::Schedule (NanoSeconds (1), &rlc);//通过Schedule事件调度机制获取rlcbuffer
    Simulator::Schedule (NanoSeconds (999999), &sinr);//通过Schedule事件调度机制在每个tti结束时获取sinr
    openGym->NotifyCurrentState (); // 启动算法获取action
    g_criedEnb.clear();
    g_arrReqs.clear();
  }
}

int
main (int argc, char *argv[])
{
  // Parameters of the scenario
  uint32_t simSeed = 15;
  double simulationTime = 30; //seconds
  // double envStepTime = 0.001; //seconds, ns3gym env step time interval
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
  
  double radius = 5000.0;//基站小区半径
  double high = 1000;//基站高度
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
  lteHelper->SetSchedulerType("ns3::AsyncFfMacScheduler");
  
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

  enbNodes.Create (numberOfenb);
  ueNodes.Create (numberOfRandomUes);

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  
  //基站的位置

  enbPositionAlloc->Add (Vector (sqrt(3) * radius/2, 0.0, high)); //eNB1

  enbPositionAlloc->Add (Vector (-sqrt(3) * radius/2, 0.0, high)); //eNB2

  // enbPositionAlloc->Add (Vector (0.0, sqrt(3) * radius/2, high)); //eNB1

  // enbPositionAlloc->Add (Vector (0.0, -sqrt(3) * radius/2, high)); //eNB2


  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (enbPositionAlloc);
  mobility.Install (enbNodes);


  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  
  //ue的位置
  uePositionAlloc->Add (Vector (-4000, 0, 0)); //
  // uePositionAlloc->Add (Vector (-4000, 0, 0)); //


  uePositionAlloc->Add (Vector (4000, 0, 0)); //
  // uePositionAlloc->Add (Vector (4000, 0, 0)); //

  //  uePositionAlloc->Add (Vector (0, -4000, 0)); //


  // uePositionAlloc->Add (Vector (0, 4000, 0)); //
  // uePositionAlloc->Add (Vector (500, 500, 0)); //


  // MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (uePositionAlloc);
  mobility.Install (ueNodes);

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
  // lteHelper->Attach(ueDevs,enbDevs.Get(0));

  for (uint16_t i = 0; i < enbDevs.GetN(); i++)
  {
    Ptr<LteEnbRrc> enbRrc = GetEnb(i)->GetRrc ();
    Ptr<AsyncFfMacScheduler> pff = GetSchedulerForEnb(i);
    pff->SetSchedDlNewTxCB(MakeBoundCallback(OnSchedDlNewTx, i));
  }

  //用trace获取物理层接收状态
  tracephyrx ();

  uint16_t dlPort = 10000;
  uint16_t ulPort = 20000;

  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.1));
  Ptr<UniformRandomVariable> stopTimeSeconds = CreateObject<UniformRandomVariable> ();
  stopTimeSeconds->SetAttribute ("Min", DoubleValue (100));
  stopTimeSeconds->SetAttribute ("Max", DoubleValue (101));

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
          dlClientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=900]"));   
          dlClientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          dlClientHelper.SetAttribute("PacketSize", UintegerValue (256));
          dlClientHelper.SetAttribute("DataRate", DataRateValue (DataRate ("2048kb/s")));
          dlClientHelper.SetAttribute("MaxBytes", UintegerValue (1024000));
          // dlClientHelper.SetAttribute ("MaxPackets", UintegerValue (100));
          // dlClientHelper.SetAttribute ("Interval", TimeValue (MilliSeconds (20.0)));
          clientApps.Add (dlClientHelper.Install (remoteHost));
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (Ipv4Address::GetAny (), dlPort));
                               
          serverApps.Add (dlPacketSinkHelper.Install (ue));


          

          OnOffHelper ulClientHelper ("ns3::UdpSocketFactory",
                                               InetSocketAddress (remoteHostAddr, ulPort));
          ulClientHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.9]"));   
          ulClientHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.01]"));
          


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
          
          // if (u % 2 == 0)
          //   {
              enum EpsBearer::Qci q = EpsBearer::GBR_GAMING;
              EpsBearer bearer (q);
             
              lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), EpsBearer(EpsBearer:: GBR_CONV_VOICE ), tft);
          //   }
          // else
          //   {
          //     lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (u), EpsBearer(EpsBearer::GBR_GAMING), tft);
          //   }

          
        }
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
  
  flowmon.InstallAll ();
  lteHelper->EnableTraces ();

  // OpenGym Env
  openGym = CreateObject<OpenGymInterface> (openGymPort);
  openGym->SetGetActionSpaceCb (MakeCallback (&MyGetActionSpace));
  openGym->SetGetObservationSpaceCb (MakeCallback (&MyGetObservationSpace));
  openGym->SetGetGameOverCb (MakeCallback (&MyGetGameOver));
  openGym->SetGetObservationCb (MakeCallback (&MyGetObservation));
  openGym->SetGetRewardCb (MakeCallback (&MyGetReward));
  openGym->SetGetExtraInfoCb (MakeCallback (&MyGetExtraInfo));
  openGym->SetExecuteActionsCb (MakeCallback (&MyExecuteActions));
  // Simulator::Schedule (Seconds (0.0), &ScheduleNextStateRead, envStepTime, openGym);
  NS_LOG_UNCOND ("Simulation start");
  Simulator::Stop (Seconds (simulationTime));
  AnimationInterface anim ("sim.xml");
  Simulator::Run ();

  NS_LOG_UNCOND ("Simulation stop");

  openGym->NotifySimulationEnd ();
  Simulator::Destroy ();
  return 0;
}
