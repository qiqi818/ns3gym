/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 */
#include <ns3/object-vector.h>
#include <ns3/object-map.h>
#include <ns3/log.h>
#include <ns3/pointer.h>
#include <ns3/math.h>
#include <ns3/string.h>
#include <ns3/simulator.h>
#include <ns3/lte-amc.h>
#include <ns3/lte-vendor-specific-parameters.h>
#include <ns3/boolean.h>
#include <cfloat>
#include <set>
#include <numeric>
#include <ns3/async-ff-mac-scheduler.h>
#include "string.h"
using namespace std;
namespace ns3 {

static vector<string> 
split2(const string& str, const string& delim) 
{  
  vector<string> res;  
  if("" == str) return res;  
  //先将要切割的字符串从string类型转换为char*类型  
  char * strs = new char[str.length() + 1] ; //不要忘了  
  strcpy(strs, str.c_str());   

  char * d = new char[delim.length() + 1];  
  strcpy(d, delim.c_str());  

  char *p = strtok(strs, d);  
  while(p) {  
    string s = p; //分割得到的字符串转换为string类型  
    res.push_back(s); //存入结果数组  
    p = strtok(NULL, d);  
  }  
	return res;  
} 


NS_LOG_COMPONENT_DEFINE ("AsyncFfMacScheduler");

/// PF type 0 allocation RBG
static const int PfType0AllocationRbg[4] = {
  10,       // RGB size 1
  26,       // RGB size 2
  63,       // RGB size 3
  110       // RGB size 4
};  // see table 7.1.6.1-1 of 36.213


NS_OBJECT_ENSURE_REGISTERED (AsyncFfMacScheduler);



AsyncFfMacScheduler::AsyncFfMacScheduler ()
  : m_cschedSapUser (0),
    m_schedSapUser (0),
    m_timeWindow (99.0),
    m_nextRntiUl (0)
{
  m_amc = CreateObject <LteAmc> ();
  m_cschedSapProvider = new MemberCschedSapProvider<AsyncFfMacScheduler> (this);
  m_schedSapProvider = new MemberSchedSapProvider<AsyncFfMacScheduler> (this);
  m_ffrSapProvider = 0;
  m_ffrSapUser = new MemberLteFfrSapUser<AsyncFfMacScheduler> (this);
}

AsyncFfMacScheduler::~AsyncFfMacScheduler ()
{
  NS_LOG_FUNCTION (this);
}

void
AsyncFfMacScheduler::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  // m_dlHarqProcessesDciBuffer.clear ();
  // m_dlHarqProcessesTimer.clear ();
  // m_dlHarqProcessesRlcPduListBuffer.clear ();
  m_dlInfoListBuffered.clear ();
  m_ulHarqCurrentProcessId.clear ();
  m_ulHarqProcessesStatus.clear ();
  m_ulHarqProcessesDciBuffer.clear ();
  delete m_cschedSapProvider;
  delete m_schedSapProvider;
  delete m_ffrSapUser;
}

TypeId
AsyncFfMacScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AsyncFfMacScheduler")
    .SetParent<FfMacScheduler> ()
    .SetGroupName("Lte")
    .AddConstructor<AsyncFfMacScheduler> ()
    .AddAttribute ("CqiTimerThreshold",
                   "The number of TTIs a CQI is valid (default 1000 - 1 sec.)",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&AsyncFfMacScheduler::m_cqiTimersThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("HarqEnabled",
                   "Activate/Deactivate the HARQ [by default is active].",
                   BooleanValue (false),
                   MakeBooleanAccessor (&AsyncFfMacScheduler::m_harqOn),
                   MakeBooleanChecker ())
    .AddAttribute ("UlGrantMcs",
                   "The MCS of the UL grant, must be [0..15] (default 0)",
                   UintegerValue (0),
                   MakeUintegerAccessor (&AsyncFfMacScheduler::m_ulGrantMcs),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("test",
                   "The MCS of the UL grant, must be [0..15] (default 0)",
                   StringValue (),
                   MakeStringAccessor (&AsyncFfMacScheduler::m_test),
                   MakeStringChecker ()) 
    .AddAttribute ("cellid",
                   "cellid",
                   StringValue ("0"),
                   MakeStringAccessor (&AsyncFfMacScheduler::m_cellid),
                   MakeStringChecker ()) 
    // .AddAttribute ("m_vec",
    //                "The MCS of the UL grant, must be [0..15] (default 0)",
    //                ObjectVectorValue (),
    //                MakeObjectVectorAccessor (&AsyncFfMacScheduler::m_vec),
    //                MakeObjectVectorChecker<uint32_t> ()) 
    // .AddAttribute ("test",
    //                "The MCS of the UL grant, must be [0..15] (default 0)",
    //                ObjectMapValue (),
    //                MakeObjectMapAccessor (&AsyncFfMacScheduler::m_flowStatsDl),
    //                MakeObjectMapChecker<asyncFlowPerf_t> ())                                                                  
  ;
  return tid;
}



void
AsyncFfMacScheduler::SetFfMacCschedSapUser (FfMacCschedSapUser* s)
{
  m_cschedSapUser = s;
}

void
AsyncFfMacScheduler::SetFfMacSchedSapUser (FfMacSchedSapUser* s)
{
  m_schedSapUser = s;
}

FfMacCschedSapProvider*
AsyncFfMacScheduler::GetFfMacCschedSapProvider ()
{
  return m_cschedSapProvider;
}

FfMacSchedSapProvider*
AsyncFfMacScheduler::GetFfMacSchedSapProvider ()
{
  return m_schedSapProvider;
}

void
AsyncFfMacScheduler::SetLteFfrSapProvider (LteFfrSapProvider* s)
{
  m_ffrSapProvider = s;
}

LteFfrSapUser*
AsyncFfMacScheduler::GetLteFfrSapUser ()
{
  return m_ffrSapUser;
}

void
AsyncFfMacScheduler::DoCschedCellConfigReq (const struct FfMacCschedSapProvider::CschedCellConfigReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  // Read the subset of parameters used
  m_cschedCellConfig = params;
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  FfMacCschedSapUser::CschedUeConfigCnfParameters cnf;
  cnf.m_result = SUCCESS;
  m_cschedSapUser->CschedUeConfigCnf (cnf);
  return;
}

void
AsyncFfMacScheduler::DoCschedUeConfigReq (const struct FfMacCschedSapProvider::CschedUeConfigReqParameters& params)
{
  NS_LOG_FUNCTION (this << " RNTI " << params.m_rnti << " txMode " << (uint16_t)params.m_transmissionMode);
  std::map <uint16_t,uint8_t>::iterator it = m_uesTxMode.find (params.m_rnti);
  if (it == m_uesTxMode.end ())
    {
      m_uesTxMode.insert (std::pair <uint16_t, double> (params.m_rnti, params.m_transmissionMode));
      // generate HARQ buffers
      // m_dlHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      // DlHarqProcessesStatus_t dlHarqPrcStatus;
      // dlHarqPrcStatus.resize (8,0);
      // m_dlHarqProcessesStatus.insert (std::pair <uint16_t, DlHarqProcessesStatus_t> (params.m_rnti, dlHarqPrcStatus));
      DlHarqProcess_t pi;
      pi.currentProcId = 0;
      memset(pi.timer, 0, sizeof(pi.timer));
      memset(pi.status, 0, sizeof(pi.status));
      memset(pi.dciBuffer, 0, sizeof(pi.dciBuffer));
      m_dlHarqProcesses.insert(std::pair<uint16_t, DlHarqProcess_t>(params.m_rnti, pi));
      // DlHarqProcessesTimer_t dlHarqProcessesTimer;
      // dlHarqProcessesTimer.resize (8,0);
      // m_dlHarqProcessesTimer.insert (std::pair <uint16_t, DlHarqProcessesTimer_t> (params.m_rnti, dlHarqProcessesTimer));
      // DlHarqProcessesDciBuffer_t dlHarqdci;
      // dlHarqdci.resize (8);
      // m_dlHarqProcessesDciBuffer.insert (std::pair <uint16_t, DlHarqProcessesDciBuffer_t> (params.m_rnti, dlHarqdci));
      // DlHarqRlcPduListBuffer_t dlHarqRlcPdu;
      // dlHarqRlcPdu.resize (2);
      // dlHarqRlcPdu.at (0).resize (8);
      // dlHarqRlcPdu.at (1).resize (8);
      // m_dlHarqProcessesRlcPduListBuffer.insert (std::pair <uint16_t, DlHarqRlcPduListBuffer_t> (params.m_rnti, dlHarqRlcPdu));

      m_ulHarqCurrentProcessId.insert (std::pair <uint16_t,uint8_t > (params.m_rnti, 0));
      UlHarqProcessesStatus_t ulHarqPrcStatus;
      ulHarqPrcStatus.resize (8,0);
      m_ulHarqProcessesStatus.insert (std::pair <uint16_t, UlHarqProcessesStatus_t> (params.m_rnti, ulHarqPrcStatus));
      UlHarqProcessesDciBuffer_t ulHarqdci;
      ulHarqdci.resize (8);
      m_ulHarqProcessesDciBuffer.insert (std::pair <uint16_t, UlHarqProcessesDciBuffer_t> (params.m_rnti, ulHarqdci));
    }
  else
    {
      (*it).second = params.m_transmissionMode;
    }
  return;
}

void
AsyncFfMacScheduler::DoCschedLcConfigReq (const struct FfMacCschedSapProvider::CschedLcConfigReqParameters& params)
{
  NS_LOG_FUNCTION (this << " New LC, rnti: "  << params.m_rnti);

  std::map <uint16_t, asyncFlowPerf_t>::iterator it;
  for (uint16_t i = 0; i < params.m_logicalChannelConfigList.size (); i++)
    {
      it = m_flowStatsDl.find (params.m_rnti);

      if (it == m_flowStatsDl.end ())
        {
          asyncFlowPerf_t flowStatsDl;
          flowStatsDl.flowStart = Simulator::Now ();
          flowStatsDl.totalBytesTransmitted = 0;
          flowStatsDl.lastTtiBytesTrasmitted = 0;
          flowStatsDl.lastAveragedThroughput = 1;
          m_flowStatsDl.insert (std::pair<uint16_t, asyncFlowPerf_t> (params.m_rnti, flowStatsDl));
          asyncFlowPerf_t flowStatsUl;
          flowStatsUl.flowStart = Simulator::Now ();
          flowStatsUl.totalBytesTransmitted = 0;
          flowStatsUl.lastTtiBytesTrasmitted = 0;
          flowStatsUl.lastAveragedThroughput = 1;
          m_flowStatsUl.insert (std::pair<uint16_t, asyncFlowPerf_t> (params.m_rnti, flowStatsUl));
        }
    }

  return;
}

void
AsyncFfMacScheduler::DoCschedLcReleaseReq (const struct FfMacCschedSapProvider::CschedLcReleaseReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  for (uint16_t i = 0; i < params.m_logicalChannelIdentity.size (); i++) {
    std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it = m_rlcBufferReq.begin ();
    std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator temp;
    while (it!=m_rlcBufferReq.end ()) {
      if (((*it).first.m_rnti == params.m_rnti) && ((*it).first.m_lcId == params.m_logicalChannelIdentity.at (i)))
      {
        temp = it;
        it++;
        m_rlcBufferReq.erase (temp);
      }
      else
      {
        it++;
      }
    }
  }
  return;
}

void
AsyncFfMacScheduler::DoCschedUeReleaseReq (const struct FfMacCschedSapProvider::CschedUeReleaseReqParameters& params)
{
  NS_LOG_FUNCTION (this);

  m_uesTxMode.erase (params.m_rnti);
  // m_dlHarqCurrentProcessId.erase (params.m_rnti);
  m_dlHarqProcesses.erase (params.m_rnti);
  // m_dlHarqProcessesStatus.erase  (params.m_rnti);
  // m_dlHarqProcessesTimer.erase (params.m_rnti);
  // m_dlHarqProcessesDciBuffer.erase  (params.m_rnti);
  // m_dlHarqProcessesRlcPduListBuffer.erase  (params.m_rnti);
  m_ulHarqCurrentProcessId.erase  (params.m_rnti);
  m_ulHarqProcessesStatus.erase  (params.m_rnti);
  m_ulHarqProcessesDciBuffer.erase  (params.m_rnti);
  m_flowStatsDl.erase  (params.m_rnti);
  m_flowStatsUl.erase  (params.m_rnti);
  m_ceBsrRxed.erase (params.m_rnti);
  std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it = m_rlcBufferReq.begin ();
  std::map<LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator temp;
  while (it!=m_rlcBufferReq.end ())
    {
      if ((*it).first.m_rnti == params.m_rnti)
        {
          temp = it;
          it++;
          m_rlcBufferReq.erase (temp);
        }
      else
        {
          it++;
        }
    }
  if (m_nextRntiUl == params.m_rnti)
    {
      m_nextRntiUl = 0;
    }

  return;
}


void
AsyncFfMacScheduler::DoSchedDlRlcBufferReq (const struct FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& params)
{
  NS_LOG_FUNCTION (this << params.m_rnti << (uint32_t) params.m_logicalChannelIdentity);
  NS_LOG_UNCOND (this << "RLC:::::" << params.m_rnti << " " << (uint32_t) params.m_logicalChannelIdentity);
  // API generated by RLC for updating RLC parameters on a LC (tx and retx queues)

  std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;

  LteFlowId_t flow (params.m_rnti, params.m_logicalChannelIdentity);

  it =  m_rlcBufferReq.find (flow);

  if (it == m_rlcBufferReq.end ())
    {
      m_rlcBufferReq.insert (std::pair <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters> (flow, params));
    }
  else
    {
      (*it).second = params;
    }

  return;
}

void
AsyncFfMacScheduler::DoSchedDlPagingBufferReq (const struct FfMacSchedSapProvider::SchedDlPagingBufferReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("method not implemented");
  return;
}

void
AsyncFfMacScheduler::DoSchedDlMacBufferReq (const struct FfMacSchedSapProvider::SchedDlMacBufferReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("method not implemented");
  return;
}

int
AsyncFfMacScheduler::GetRbgSize (int dlbandwidth)
{
  for (int i = 0; i < 4; i++)
    {
      if (dlbandwidth < PfType0AllocationRbg[i])
        {
          return (i + 1);
        }
    }

  return (-1);
}


unsigned int
AsyncFfMacScheduler::LcActivePerFlow (uint16_t rnti)
{
  std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator it;
  unsigned int lcActive = 0;
  for (it = m_rlcBufferReq.begin (); it != m_rlcBufferReq.end (); it++) {
    if ((*it).first.m_rnti > rnti) break;
    if ((*it).first.m_rnti != rnti) continue;
    const FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& req = it->second;
    if((req.m_rlcTransmissionQueueSize > 0) || (req.m_rlcRetransmissionQueueSize > 0) || (req.m_rlcStatusPduSize > 0))
    {
      lcActive++;
    }
  }
  return (lcActive);
}


uint8_t
AsyncFfMacScheduler::HarqProcessAvailability (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);

  if (m_dlHarqProcesses.count (rnti) == 0) {
    NS_FATAL_ERROR ("No Process Id Statusfound for this RNTI " << rnti);
  }

  uint8_t i = m_dlHarqProcesses[rnti].currentProcId;
  do {
    i = (i + 1) % HARQ_PROC_NUM;
  }
  while ( (m_dlHarqProcesses[rnti].status[i] != 0)&&(i != m_dlHarqProcesses[rnti].currentProcId));
  if (m_dlHarqProcesses[rnti].status[i] == 0) {
    return (true);
  }
  else
  {
    return (false); // return a not valid harq proc id
  }
}


uint8_t
AsyncFfMacScheduler::UpdateHarqProcessId (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  if (m_harqOn == false) {
   return (0);
  }

  if (m_dlHarqProcesses.count (rnti) == 0) {
    NS_FATAL_ERROR ("No Process Id Statusfound for this RNTI " << rnti);
  }
  DlHarqProcess_t& proc = m_dlHarqProcesses[rnti];
  uint8_t i = proc.currentProcId;
  do {
    i = (i + 1) % HARQ_PROC_NUM;
  } while((proc.status[i] != 0)&&(i != proc.currentProcId));

  if (proc.status[i] == 0) {
    proc.currentProcId = i;
    proc.status[i] = 1;
  }
  else {
    NS_FATAL_ERROR ("No HARQ process available for RNTI " << rnti << " check before update with HarqProcessAvailability");
  }

  return proc.currentProcId;
}


void
AsyncFfMacScheduler::RefreshHarqProcesses ()
{
  NS_LOG_FUNCTION (this);

  std::map <uint16_t, DlHarqProcess_t>::iterator it;
  for (it = m_dlHarqProcesses.begin (); it != m_dlHarqProcesses.end (); it++)
  {
    uint16_t rnti = (*it).first;
    DlHarqProcess_t& proc = (*it).second;
    for (uint16_t i = 0; i < HARQ_PROC_NUM; i++)
    {
      if (proc.timer[i] == HARQ_DL_TIMEOUT) {
        // reset HARQ process
        NS_LOG_DEBUG (this << " Reset HARQ proc " << i << " for RNTI " << rnti);
        proc.status[i] = 0;
        proc.timer[i] = 0;
      }
      else {
        proc.timer[i]++;
      }
    }
  }
}

void 
AsyncFfMacScheduler::SetSchedDlNewTxCB(
  Callback<void, const std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>& > cb)
{
  m_cb = cb;
}

void AsyncFfMacScheduler::DoSchedRar (std::vector<struct BuildRarListElement_s>& ret)
{
  // RACH Allocation
  uint16_t rbAllocatedNum = 0;
  uint8_t maxContinuousUlBandwidth = 0;
  uint8_t tmpMinBandwidth = 0;
  uint16_t ffrRbStartOffset = 0;
  uint16_t tmpFfrRbStartOffset = 0;
  uint16_t index = 0;
  
  std::vector<bool> ulRbMap = m_ffrSapProvider->GetAvailableUlRbg ();
  for (std::vector<bool>::iterator it = ulRbMap.begin (); it != ulRbMap.end (); it++)
  {
    if ((*it) == true) { // the rbg specified by *it* cannot be used
      rbAllocatedNum++;
      if (tmpMinBandwidth > maxContinuousUlBandwidth)
      {
        maxContinuousUlBandwidth = tmpMinBandwidth;
        ffrRbStartOffset = tmpFfrRbStartOffset;
      }
      tmpMinBandwidth = 0;
    }
    else {
      if (tmpMinBandwidth == 0)
      {
        tmpFfrRbStartOffset = index;
      }
      tmpMinBandwidth++;
    }
    index++;
  }

  if (tmpMinBandwidth > maxContinuousUlBandwidth) {
    maxContinuousUlBandwidth = tmpMinBandwidth;
    ffrRbStartOffset = tmpFfrRbStartOffset;
  }

  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  uint16_t rbStart = ffrRbStartOffset;
  std::vector<struct RachListElement_s>::iterator itRach;
  for (itRach = m_rachList.begin (); itRach != m_rachList.end (); itRach++)
  {
    NS_ASSERT_MSG (m_amc->GetUlTbSizeFromMcs (m_ulGrantMcs, m_cschedCellConfig.m_ulBandwidth) > (*itRach).m_estimatedSize,
                   " Default UL Grant MCS does not allow to send RACH messages");
    BuildRarListElement_s newRar;
    newRar.m_rnti = (*itRach).m_rnti;
    // DL-RACH Allocation
    // Ideal: no needs of configuring m_dci
    // UL-RACH Allocation
    newRar.m_grant.m_rnti = newRar.m_rnti;
    newRar.m_grant.m_mcs = m_ulGrantMcs;
    uint16_t rbLen = 1;
    uint16_t tbSizeBits = 0;
    // find lowest TB size that fits UL grant estimated size
    while ((tbSizeBits < (*itRach).m_estimatedSize) &&
            (rbStart + rbLen < (ffrRbStartOffset + maxContinuousUlBandwidth)))
    {
      rbLen++;
      tbSizeBits = m_amc->GetUlTbSizeFromMcs (m_ulGrantMcs, rbLen);
    }

    if (tbSizeBits < (*itRach).m_estimatedSize)
    {
      // no more allocation space: finish allocation
      break;
    }

    newRar.m_grant.m_rbStart = rbStart;
    newRar.m_grant.m_rbLen = rbLen;
    newRar.m_grant.m_tbSize = tbSizeBits / 8;
    newRar.m_grant.m_hopping = false;
    newRar.m_grant.m_tpc = 0;
    newRar.m_grant.m_cqiRequest = false;
    newRar.m_grant.m_ulDelay = false;
    NS_LOG_INFO (this << " UL grant allocated to RNTI " << (*itRach).m_rnti << " rbStart "
                      << rbStart << " rbLen " << rbLen << " MCS " << m_ulGrantMcs << " tbSize "
                      << newRar.m_grant.m_tbSize);
    for (uint16_t i = rbStart; i < rbStart + rbLen; i++)
    {
      m_rachAllocationMap.at (i) = (*itRach).m_rnti;
    }

    if (m_harqOn == true)
    {
      // generate UL-DCI for HARQ retransmissions
      UlDciListElement_s uldci;
      uldci.m_rnti  = newRar.m_rnti;
      uldci.m_rbLen = rbLen;
      uldci.m_rbStart = rbStart;
      uldci.m_mcs = m_ulGrantMcs;
      uldci.m_tbSize = tbSizeBits / 8;
      uldci.m_ndi = 1;
      uldci.m_cceIndex = 0;
      uldci.m_aggrLevel = 1;
      uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
      uldci.m_hopping = false;
      uldci.m_n2Dmrs = 0;
      uldci.m_tpc = 0; // no power control
      uldci.m_cqiRequest = false; // only period CQI at this stage
      uldci.m_ulIndex = 0; // TDD parameter
      uldci.m_dai = 1; // TDD parameter
      uldci.m_freqHopping = 0;
      uldci.m_pdcchPowerOffset = 0; // not used

      uint8_t harqId = 0;
      std::map<uint16_t, uint8_t>::iterator itProcId;
      itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
      if (itProcId == m_ulHarqCurrentProcessId.end ()) {
        NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
      }
      harqId = (*itProcId).second;
      std::map<uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci =
          m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
      if (itDci == m_ulHarqProcessesDciBuffer.end ()) {
        NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI "
                        << uldci.m_rnti);
      }
      (*itDci).second.at (harqId) = uldci;
    }

    rbStart = rbStart + rbLen;
    ret.push_back (newRar);
  }
  m_rachList.clear ();
}

void 
AsyncFfMacScheduler::ResetHarq(uint16_t rnti, uint8_t harqId)
{
  if (m_dlHarqProcesses.count (rnti) == 0) {
    NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << rnti);
    return;
  }
  m_dlHarqProcesses[rnti].status[harqId] = 0;
  m_dlHarqProcesses[rnti].timer[harqId] = 0;
  for (uint16_t k = 0; k < MAX_LAYER; k++)
  {
    m_dlHarqProcesses[rnti].rlcPduBuffer[harqId][k].clear();
  }
}

std::vector<uint16_t> 
AsyncFfMacScheduler::Bitmap2Alloc(uint32_t bitmap)
{
  std::vector<uint16_t> dciRbg;
  uint32_t mask = 0x1;
  NS_LOG_INFO ("Original RBGs " << bitmap);
  for (int j = 0; j < 32; j++) {
    if (((bitmap & mask) >> j) == 1)
    {
      dciRbg.push_back (j);
      NS_LOG_INFO ("\t" << j);
    }
    mask = (mask << 1);
  }
  return dciRbg;
}

uint32_t 
AsyncFfMacScheduler::Alloc2Bitmap(const std::vector<uint16_t>& alloc)
{
  uint32_t rbgMask = 0;
  for (uint16_t k = 0; k < alloc.size (); k++) {
    rbgMask = rbgMask + (0x1 << alloc[k]);
    // NS_LOG_INFO (this << " Allocated RBG " << alloc[k]);
  }
  return rbgMask; // (32 bit bitmap see 7.1.6 of 36.213)
}

void
AsyncFfMacScheduler::DoSchedDlRetx(std::vector<bool> &rbgMap,
                                    std::vector<struct BuildDataListElement_s>& ret)
{
  std::set<uint16_t> rntiAllocated;
  std::vector<struct DlInfoListElement_s> dlInfoListUntxed;
  for (uint16_t i = 0; i < m_dlInfoListBuffered.size (); i++) 
  {
    uint16_t rnti = m_dlInfoListBuffered[i].m_rnti;
    uint8_t harqId = m_dlInfoListBuffered[i].m_harqProcessId;
    if (rntiAllocated.count (rnti) != 0) {
      // 每个TTI针对每个RNTI的重传只能调度一次？
      // RNTI already allocated for retx
      continue;
    }
    
    uint8_t nLayers = m_dlInfoListBuffered[i].m_harqStatus.size ();
    std::vector<bool> need_retx;
    NS_LOG_INFO (this << " Processing DLHARQ feedback");
    need_retx.push_back (m_dlInfoListBuffered[i].m_harqStatus[0] == DlInfoListElement_s::NACK);
    if (nLayers == 1) {
      need_retx.push_back (false);
    }
    else {
      need_retx.push_back (m_dlInfoListBuffered[i].m_harqStatus[1] == DlInfoListElement_s::NACK);
    }

    if(!need_retx[0] && !need_retx[1]) {
      // update HARQ process status
      NS_LOG_INFO (this << " HARQ received ACK for UE " << rnti);
      ResetHarq(rnti, harqId);
      continue;
    }

    // need retx
    // retrieve HARQ process information
    NS_LOG_INFO (this << " HARQ retx RNTI " << rnti << " harqId " << (uint16_t) harqId);
    if (m_dlHarqProcesses.count (rnti) == 0) {
      NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << rnti);
    }

    DlDciListElement_s dci = m_dlHarqProcesses[rnti].dciBuffer[harqId];
    int rv = dci.m_rv[0];
    if (dci.m_rv.size () > 1) {
      rv = (dci.m_rv[0] > dci.m_rv[1]) ? dci.m_rv[0] : dci.m_rv[1];
    }

    if (rv == 3)
    {
      // maximum number of retx reached -> drop process
      NS_LOG_INFO ("Maximum number of retransmissions reached -> drop process");
      ResetHarq(rnti, harqId);
      continue;
    }

    // check the feasibility of retransmitting on the same RBGs
    // translate the DCI to Spectrum framework
    std::vector<uint16_t> dciRbg = Bitmap2Alloc(dci.m_rbBitmap);
    bool free = true;
    for (uint8_t j = 0; j < dciRbg.size (); j++) {
      if (rbgMap[dciRbg[j]] == true) {
        free = false;
        break;
      }
    }

    if (free) { // 重传所需要的RBG全部未被占用
      // use the same RBGs for the retx reserve RBGs
      for (uint8_t j = 0; j < dciRbg.size (); j++) {
        rbgMap[dciRbg[j]] = true;
        NS_LOG_INFO ("RBG " << dciRbg[j] << " assigned");
      }
      NS_LOG_INFO (this << " Send retx in the same RBGs");
    }
    else { // 重传所需要的RBG部分已被占用
      // find RBGs for sending HARQ retx
      uint8_t j = 0;
      uint8_t rbgId = (dciRbg[dciRbg.size () - 1] + 1) % rbgMap.size (); //qinhao, may be wrong
      uint8_t startRbg = dciRbg[dciRbg.size () - 1];
      std::vector<bool> rbgMapCopy = rbgMap;
      while ((j < dciRbg.size ()) && (startRbg != rbgId)) {
        if (rbgMapCopy[rbgId] == false) {
          rbgMapCopy[rbgId] = true;
          dciRbg[j] = rbgId;
          j++;
        }
        rbgId = (rbgId + 1) % rbgMap.size (); //qinhao ma
      }

      if (j == dciRbg.size ()) {
        // find new RBGs -> update DCI map
        dci.m_rbBitmap = Alloc2Bitmap(dciRbg);
        rbgMap = rbgMapCopy;
        NS_LOG_INFO (this << " Move retx in RBGs " << dciRbg.size ());
      }
      else {
        // HARQ retx cannot be performed on this TTI -> store it
        dlInfoListUntxed.push_back (m_dlInfoListBuffered[i]);
        NS_LOG_INFO (this << " No resource for this retx -> buffer it");
        continue; // qinhao
      }
    }

    // retrieve RLC PDU list for retx TBsize and update DCI
    BuildDataListElement_s newEl;
    for (uint8_t j = 0; j < nLayers; j++)
    {
      if(need_retx.at (j))
      {
        if (j >= dci.m_ndi.size ())
        {
          // for avoiding errors in MIMO transient phases
          dci.m_ndi.push_back (0);
          dci.m_rv.push_back (0);
          dci.m_mcs.push_back (0);
          dci.m_tbsSize.push_back (0);
          NS_LOG_INFO (this << " layer " << (uint16_t) j << " no txed (MIMO transition)");
        }
        else
        {
          dci.m_ndi.at (j) = 0;
          dci.m_rv.at (j)++;
          // DlHarqProcessesDciBuffer_t &dhdb = m_dlHarqProcessesDciBuffer[rnti];
          // DlDciListElement_s dci = dhdb[harqId];
          m_dlHarqProcesses[rnti].dciBuffer[harqId].m_rv[j]++;
          NS_LOG_INFO (this << " layer " << (uint16_t) j << " RV "
                            << (uint16_t) dci.m_rv.at (j));
        }
      }
      else
      {
        // empty TB of layer j
        dci.m_ndi.at (j) = 0;
        dci.m_rv.at (j) = 0;
        dci.m_mcs.at (j) = 0;
        dci.m_tbsSize.at (j) = 0;
        NS_LOG_INFO (this << " layer " << (uint16_t) j << " no retx");
      }
    }

    for (uint16_t k = 0; k < m_dlHarqProcesses[rnti].rlcPduBuffer[dci.m_harqProcess][0].size (); k++)
    // for (uint16_t k = 0; k < (*itRlcPdu).second.at (0).at (dci.m_harqProcess).size (); k++)
    {
      std::vector<struct RlcPduListElement_s> rlcPduListPerLc;
      for (uint8_t j = 0; j < nLayers; j++)
      {
        if (need_retx.at (j))
        {
          if (j < dci.m_ndi.size ())
          {
            NS_LOG_INFO (" layer " << (uint16_t) j << " tb size " << dci.m_tbsSize.at (j));
            rlcPduListPerLc.push_back (m_dlHarqProcesses[rnti].rlcPduBuffer[dci.m_harqProcess][j][k]);
          }
        }
        else
        {
          // if no retx needed on layer j, push an RlcPduListElement_s object with m_size=0 to keep the size of rlcPduListPerLc vector = 2 in case of MIMO
          NS_LOG_INFO (" layer " << (uint16_t) j << " tb size " << dci.m_tbsSize.at (j));
          RlcPduListElement_s emptyElement;
          emptyElement.m_logicalChannelIdentity = m_dlHarqProcesses[rnti].rlcPduBuffer[dci.m_harqProcess][j][k].m_logicalChannelIdentity;
          emptyElement.m_size = 0;
          rlcPduListPerLc.push_back (emptyElement);
        }
      }

      if (rlcPduListPerLc.size () > 0)
      {
        newEl.m_rlcPduList.push_back (rlcPduListPerLc);
      }
    }

    newEl.m_rnti = rnti;
    newEl.m_dci = dci;
    m_dlHarqProcesses[rnti].dciBuffer[harqId].m_rv = dci.m_rv;

    // refresh timer
    m_dlHarqProcesses[rnti].timer[harqId] = 0;
    ret.push_back (newEl);
    rntiAllocated.insert (rnti);
  }
  m_dlInfoListBuffered.clear ();
  m_dlInfoListBuffered = dlInfoListUntxed;
}

std::vector <uint8_t>
AsyncFfMacScheduler::GetWorstCqi(uint16_t rnti, const std::vector<uint16_t>& alloc)
{
  int nLayer = TransmissionModesLayers::TxMode2LayerNum(m_uesTxMode[rnti]);
  std::vector <uint8_t> worstCqi(nLayer, 15);

  if(m_a30CqiRxed.count(rnti) == 0) {
    for (uint8_t j = 0; j < nLayer; j++) {
      worstCqi[j] = 1; // try with lowest MCS in RBG with no info on channel
    }
    return worstCqi;
  }

  for (uint16_t k = 0; k < alloc.size (); k++)
  {
    uint16_t rbg = alloc[k];
    if (m_a30CqiRxed[rnti].m_higherLayerSelected.size () > rbg) {
      NS_LOG_INFO (this << " RBG " << rbg << " CQI " << (uint16_t)m_a30CqiRxed[rnti].m_higherLayerSelected[rbg].m_sbCqi[0]);
      for (uint8_t j = 0; j < nLayer; j++) {
        if (m_a30CqiRxed[rnti].m_higherLayerSelected[rbg].m_sbCqi.size () > j) {
          if ((m_a30CqiRxed[rnti].m_higherLayerSelected[rbg].m_sbCqi[j]) < worstCqi[j])
          {
            worstCqi[j] = (m_a30CqiRxed[rnti].m_higherLayerSelected[rbg].m_sbCqi[j]);
          }
        }
        else {
          // no CQI for this layer of this suband -> worst one
          worstCqi[j] = 1;
        }
      }
    }
    else {
      for (uint8_t j = 0; j < nLayer; j++) {
        worstCqi[j] = 1; // try with lowest MCS in RBG with no info on channel
      }
    }
  }

  return worstCqi;
}

BuildDataListElement_s
AsyncFfMacScheduler::BuildFromAllocation(uint16_t rnti, const std::vector<uint16_t>& alloc)
{
  int rbgSize = GetRbgSize (m_cschedCellConfig.m_dlBandwidth);
  std::map <uint16_t,uint8_t>::iterator itTxMode;
  itTxMode = m_uesTxMode.find (rnti);
  if (itTxMode == m_uesTxMode.end ()) {
    NS_FATAL_ERROR ("No Transmission Mode info on user " << rnti);
  }
  int nLayer = TransmissionModesLayers::TxMode2LayerNum((*itTxMode).second);

  BuildDataListElement_s newEl;
  newEl.m_rnti = rnti;
  
  // create the DlDciListElement_s
  DlDciListElement_s newDci;
  newDci.m_rnti = rnti;
  newDci.m_harqProcess = UpdateHarqProcessId (rnti);
  newDci.m_resAlloc = 0;  // only allocation type 0 at this stage
  newDci.m_rbBitmap = Alloc2Bitmap(alloc); // (32 bit bitmap see 7.1.6 of 36.213)
  newDci.m_tpc = m_ffrSapProvider->GetTpc (rnti);
  for (uint8_t j = 0; j < nLayer; j++) {
    newDci.m_ndi.push_back (1);
    newDci.m_rv.push_back (0);
  }

  uint16_t lcActives = LcActivePerFlow (rnti);
  NS_LOG_INFO (this << "Allocate user " << newEl.m_rnti << " rbg " << lcActives);
  if (lcActives == 0) {
    // Set to max value, to avoid divide by 0 below
    lcActives = (uint16_t)65535; // UINT16_MAX;
  }

  std::vector <uint8_t> worstCqi = GetWorstCqi(rnti, alloc);
  for (uint8_t j = 0; j < nLayer; j++)
  {
    NS_LOG_INFO (this << " Layer " << (uint16_t)j << " CQI selected " << (uint16_t)worstCqi[j]);
    newDci.m_mcs.push_back (m_amc->GetMcsFromCqi (worstCqi[j]));
    int tbSize = (m_amc->GetDlTbSizeFromMcs (newDci.m_mcs[j], alloc.size() * rbgSize) / 8); // (size of TB in bytes according to table 7.1.7.2.1-1 of 36.213)
    newDci.m_tbsSize.push_back (tbSize);
    NS_LOG_INFO (this << " Layer " << (uint16_t)j << " MCS selected" << m_amc->GetMcsFromCqi(worstCqi[j]));
  }

  // create the rlc PDUs -> equally divide resources among actives LCs
  std::map <LteFlowId_t, FfMacSchedSapProvider::SchedDlRlcBufferReqParameters>::iterator itBufReq;
  for (itBufReq = m_rlcBufferReq.begin (); itBufReq != m_rlcBufferReq.end (); itBufReq++)
  {
    if ((*itBufReq).first.m_rnti > rnti) break;
    if ((*itBufReq).first.m_rnti != rnti) continue;
    const FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& req = (*itBufReq).second;
    if((req.m_rlcTransmissionQueueSize > 0) || (req.m_rlcRetransmissionQueueSize > 0) || (req.m_rlcStatusPduSize > 0))
    {
      std::vector <struct RlcPduListElement_s> newRlcPduLe;
      for (uint8_t j = 0; j < nLayer; j++) {
        RlcPduListElement_s newRlcEl;
        newRlcEl.m_logicalChannelIdentity = (*itBufReq).first.m_lcId;
        newRlcEl.m_size = newDci.m_tbsSize[j] / lcActives;
        NS_LOG_INFO (this << " LCID " << (uint32_t) newRlcEl.m_logicalChannelIdentity << " size " << newRlcEl.m_size << " layer " << (uint16_t)j);
        newRlcPduLe.push_back (newRlcEl);
        UpdateDlRlcBufferInfo (newDci.m_rnti, newRlcEl.m_logicalChannelIdentity, newRlcEl.m_size);
        if (m_harqOn == true) {
          // store RLC PDU list for HARQ
          m_dlHarqProcesses[rnti].rlcPduBuffer[newDci.m_harqProcess][j].push_back (newRlcEl);
        }
      }
      newEl.m_rlcPduList.push_back (newRlcPduLe);
    }
  }
  newEl.m_dci = newDci;
  if (m_harqOn == true) {
    // store DCI for HARQ
    if (m_dlHarqProcesses.count(rnti) == 0) {
      NS_FATAL_ERROR("Unable to find RNTI entry in DCI HARQ buffer for RNTI " << rnti);
    }
    m_dlHarqProcesses[rnti].dciBuffer[newDci.m_harqProcess] = newDci;
    m_dlHarqProcesses[rnti].timer[newDci.m_harqProcess] = 0;
  }
  // ...more parameters -> ignored in this version
  return newEl;
}

void
AsyncFfMacScheduler::DoSchedDlTriggerReq (const struct FfMacSchedSapProvider::SchedDlTriggerReqParameters& params)
{
  NS_LOG_FUNCTION (this << " Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
  NS_LOG_UNCOND (this << " Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
  // API generated by RLC for triggering the scheduling of a DL subframe

  // evaluate the relative channel quality indicator for each UE per each RBG
  // (since we are using allocation type 0 the small unit of allocation is RBG)
  // Resource allocation type 0 (see sec 7.1.6.1 of 36.213)

  RefreshDlCqiMaps ();

  int rbgSize = GetRbgSize (m_cschedCellConfig.m_dlBandwidth);
  std::vector<bool> rbgMap = m_ffrSapProvider->GetAvailableDlRbg (); // global RBGs map

  int rbgNum = m_cschedCellConfig.m_dlBandwidth / rbgSize;
  NS_ASSERT_MSG ((size_t)rbgNum == rbgMap.size (), "rbgNum == rbgMap.size()++++++++++++++++++++++++++");
  // update UL HARQ proc id
  std::map <uint16_t, uint8_t>::iterator itProcId;
  for (itProcId = m_ulHarqCurrentProcessId.begin (); itProcId != m_ulHarqCurrentProcessId.end (); itProcId++)
  {
    (*itProcId).second = ((*itProcId).second + 1) % HARQ_PROC_NUM;
  }

  FfMacSchedSapUser::SchedDlConfigIndParameters ret;
  DoSchedRar (ret.m_buildRarList);

  // Process DL HARQ feedback
  RefreshHarqProcesses ();

  // retrieve past HARQ retx buffered
  if (params.m_dlInfoList.size () > 0) {
    NS_LOG_INFO (this << " Received DL-HARQ feedback");
    m_dlInfoListBuffered.insert(m_dlInfoListBuffered.end(), params.m_dlInfoList.begin(), params.m_dlInfoList.end());
  }

  if (m_harqOn == false) { // Ignore HARQ feedback
    m_dlInfoListBuffered.clear ();
  }

  DoSchedDlRetx(rbgMap, ret.m_buildDataList);

  uint16_t rbgAllocatedNum = 0;
  for (size_t idx = 0; idx < rbgMap.size(); idx++) {
    // the rbg specified by *it* cannot be used
    if(rbgMap[idx] == true) rbgAllocatedNum++;
  }

  if (rbgAllocatedNum == rbgMap.size()) {
    // all the RBGs are already allocated -> exit
    if ((ret.m_buildDataList.size () > 0) || (ret.m_buildRarList.size () > 0))
    {
      m_schedSapUser->SchedDlConfigInd (ret);
      m_ret_p2 = m_ret_p;
      m_ret_p = m_ret;
      m_ret = ret;
    }
    return;
  }

  StartSchedDlNewTx(ret);

  Simulator::ScheduleNow(&AsyncFfMacScheduler::ExecuteDlSchedResult, this);
}

void
AsyncFfMacScheduler::StartSchedDlNewTx(FfMacSchedSapUser::SchedDlConfigIndParameters ret)
{
  m_ret = ret;
  m_cb(m_rlcBufferReq);
  // 这里先返回，等获得RLC部分的调度结果后再统一执行
}

void  
AsyncFfMacScheduler::ExecuteDlSchedResult()
{
  cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&****************************" << endl;
  //----------------------------------------------------------------------------------------------------------
  //改写的调度执行分配结果的过程，将传递进来的保存有动作（资源分配信息的）参数m_test转化为调度信息执行下去
  std::set<uint16_t> rntiAllocated; // save all the rnti scheduled for retx
  for(size_t ii = 0; ii < m_ret.m_buildDataList.size(); ii++) {
    rntiAllocated.insert(m_ret.m_buildDataList[ii].m_rnti);
  }

  std::map <uint16_t, std::vector <uint16_t> > allocationMap; // RBs map per RNTI
  vector<string> vs = split2(m_test," ");//分割字符串
  //每两个为一组(RNTIi,资源编号i)进行执行
  for(uint16_t i = 0; i < vs.size();i+=2)
  {
    uint16_t rnti = stoi(vs[i]);
    int rbg = stoi(vs[i+1]);

    if(rnti == 0) continue;

    if ((m_ffrSapProvider->IsDlRbgAvailableForUe (rbg, rnti) == false)) {
      cout << "CellId为 " << stoi(m_cellid)+1 << " ,RNTI为 " << rnti << " 的UE不能占用rbg编号" << rbg << endl;
      continue;
    }
    
    if ((rntiAllocated.count(rnti) > 0)|| (!HarqProcessAvailability (rnti))) { //qinhao 
      cout << "CellId为 " << stoi(m_cellid)+1 << " ,RNTI为 " << rnti << " 的UE重传" << endl;
      continue;
    }
      
    if (LcActivePerFlow (rnti) > 0) {
      cout << "CellId为 " << stoi(m_cellid)+1 << " ,RNTI为 " << rnti << " 的UE分配到rbg编号 " << rbg << endl;
#if 1      
      allocationMap[rnti].push_back(rbg);
#else      
      std::map <uint16_t, std::vector <uint16_t> >::iterator itMap;
      //查找allocationMap中是否已有RNTI为rnti的ue的调度信息itMap
      itMap = allocationMap.find (rnti);
      if(allocationMap.count (rnti) == 0) {
        //若无，则创建它的调度信息itMap
        // insert new element
        std::vector <uint16_t> tempMap;
        tempMap.push_back (rbg);//将资源编号保存在tempMap中
        //[[RNTI1,[资源编号1，资源编号2，...]],[RNTI2,[资源编号1，资源编号2，...],...]  -->  [[RNTI1,tempMap1],[RNTI2,tempMap2],...]  -->  [itMap1,itMap2,...]  -->  allocationMap
        allocationMap.insert (std::pair <uint16_t, std::vector <uint16_t> > (rnti, tempMap));
      }
      else {
        //若已有RNTI为stoi(vs[i])的ue的调度信息，则直接将追加的资源编号push到tempMap中
        (*itMap).second.push_back (rbg);
      }            
#endif      
    }
  }
  cout << endl;
            
  //----------------------------------------------------------------------------------------------------------------

  // 根据allocationMap生成调度结果ret.m_buildDataList
  // generate the transmission opportunities by grouping the RBGs of the same RNTI and
  // creating the correspondent DCIs
  std::map <uint16_t, std::vector <uint16_t> >::iterator itMap = allocationMap.begin ();
  while (itMap != allocationMap.end ())
  {
    // create new BuildDataListElement_s for this LC
    uint16_t rnti = (*itMap).first;
    const std::vector<uint16_t>& alloc = (*itMap).second;

    BuildDataListElement_s newEl = BuildFromAllocation(rnti, alloc);
    m_ret.m_buildDataList.push_back (newEl);

    uint32_t bytesTxed = std::accumulate(newEl.m_dci.m_tbsSize.begin(), newEl.m_dci.m_tbsSize.end(), 0);
    // update UE stats
    m_flowStatsDl[rnti].lastTtiBytesTrasmitted = bytesTxed;
    NS_LOG_INFO (this << " UE total bytes txed " << bytesTxed);
    itMap++;
  } // end while allocation
  m_ret.m_nrOfPdcchOfdmSymbols = 1;   /// \todo check correct value according the DCIs txed

  // update UEs stats
  NS_LOG_INFO (this << " Update UEs statistics");
  std::map <uint16_t, asyncFlowPerf_t>::iterator itStats;
  for (itStats = m_flowStatsDl.begin (); itStats != m_flowStatsDl.end (); itStats++)
  {
    (*itStats).second.totalBytesTransmitted += (*itStats).second.lastTtiBytesTrasmitted;
    // update average throughput (see eq. 12.3 of Sec 12.3.1.2 of LTE – The UMTS Long Term Evolution, Ed Wiley)
    (*itStats).second.lastAveragedThroughput = ((1.0 - (1.0 / m_timeWindow)) * (*itStats).second.lastAveragedThroughput) + ((1.0 / m_timeWindow) * (double)((*itStats).second.lastTtiBytesTrasmitted / 0.001));
    NS_LOG_INFO (this << " UE total bytes " << (*itStats).second.totalBytesTransmitted);
    NS_LOG_INFO (this << " UE average throughput " << (*itStats).second.lastAveragedThroughput);
    (*itStats).second.lastTtiBytesTrasmitted = 0;
  }

  m_schedSapUser->SchedDlConfigInd (m_ret);
}

void
AsyncFfMacScheduler::DoSchedDlRachInfoReq (const struct FfMacSchedSapProvider::SchedDlRachInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  m_rachList = params.m_rachList;
}

void
AsyncFfMacScheduler::DoSchedDlCqiInfoReq (const struct FfMacSchedSapProvider::SchedDlCqiInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  m_ffrSapProvider->ReportDlCqiInfo (params);

  for (unsigned int i = 0; i < params.m_cqiList.size (); i++)
  {
    uint16_t rnti = params.m_cqiList[i].m_rnti;
    if( params.m_cqiList[i].m_cqiType == CqiListElement_s::P10 ) { // 周期上报CQI，通过PUCCH
      NS_LOG_LOGIC ("wideband CQI " <<  (uint32_t) params.m_cqiList[i].m_wbCqi[0] << " reported");
      m_p10CqiRxed[rnti] = params.m_cqiList[i].m_wbCqi[0]; // only codeword 0 at this stage (SISO)
      // generate correspondent timer
      m_p10CqiTimers[rnti] = m_cqiTimersThreshold;
    }
    else if( params.m_cqiList[i].m_cqiType == CqiListElement_s::A30 ) { // 非周期上报，通过PUSCH
      // subband CQI reporting high layer configured
      m_a30CqiRxed[rnti] = params.m_cqiList[i].m_sbMeasResult;
      m_a30CqiTimers[rnti] = m_cqiTimersThreshold;
    }
    else {
      NS_LOG_ERROR (this << " CQI type unknown");
    }
  }
  return;
}

double
AsyncFfMacScheduler::EstimateUlSinr (uint16_t rnti, uint16_t rb)
{
  std::map <uint16_t, std::vector <double> >::iterator itCqi = m_ueCqi.find (rnti);
  if (itCqi == m_ueCqi.end ())
    {
      // no cqi info about this UE
      return (NO_SINR);

    }
  else
    {
      // take the average SINR value among the available
      double sinrSum = 0;
      unsigned int sinrNum = 0;
      for (uint32_t i = 0; i < m_cschedCellConfig.m_ulBandwidth; i++)
        {
          double sinr = (*itCqi).second.at (i);
          if (sinr != NO_SINR)
            {
              sinrSum += sinr;
              sinrNum++;
            }
        }
      double estimatedSinr = (sinrNum > 0) ? (sinrSum / sinrNum) : DBL_MAX;
      // store the value
      (*itCqi).second.at (rb) = estimatedSinr;
      return (estimatedSinr);
    }
}

void
AsyncFfMacScheduler::DoSchedUlTriggerReq (const struct FfMacSchedSapProvider::SchedUlTriggerReqParameters& params)
{
 
  NS_LOG_FUNCTION (this << " UL - Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf) << " size " << params.m_ulInfoList.size ());

  RefreshUlCqiMaps ();
  m_ffrSapProvider->ReportUlCqiInfo (m_ueCqi);


  // Generate RBs map
  FfMacSchedSapUser::SchedUlConfigIndParameters ret;
  std::vector <bool> rbMap;
  uint16_t rbAllocatedNum = 0;
  std::set <uint16_t> rntiAllocated;
  std::vector <uint16_t> rbgAllocationMap;
  // update with RACH allocation map
  rbgAllocationMap = m_rachAllocationMap;
  //rbgAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);
  m_rachAllocationMap.clear ();
  m_rachAllocationMap.resize (m_cschedCellConfig.m_ulBandwidth, 0);

  rbMap.resize (m_cschedCellConfig.m_ulBandwidth, false);
  rbMap = m_ffrSapProvider->GetAvailableUlRbg ();

  for (std::vector<bool>::iterator it = rbMap.begin (); it != rbMap.end (); it++)
    {
      if ((*it) == true )
        {
          rbAllocatedNum++;
        }
    }

  uint8_t minContinuousUlBandwidth = m_ffrSapProvider->GetMinContinuousUlBandwidth ();
  uint8_t ffrUlBandwidth = m_cschedCellConfig.m_ulBandwidth - rbAllocatedNum;

  // remove RACH allocation
  for (uint16_t i = 0; i < m_cschedCellConfig.m_ulBandwidth; i++)
    {
      if (rbgAllocationMap.at (i) != 0)
        {
          rbMap.at (i) = true;
          NS_LOG_DEBUG (this << " Allocated for RACH " << i);
        }
    }


  if (m_harqOn == true)
    {
      //   Process UL HARQ feedback

      for (uint16_t i = 0; i < params.m_ulInfoList.size (); i++)
        {
          if (params.m_ulInfoList.at (i).m_receptionStatus == UlInfoListElement_s::NotOk)
            {
              // retx correspondent block: retrieve the UL-DCI
              uint16_t rnti = params.m_ulInfoList.at (i).m_rnti;
              std::map <uint16_t, uint8_t>::iterator itProcId = m_ulHarqCurrentProcessId.find (rnti);
              if (itProcId == m_ulHarqCurrentProcessId.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                }
              uint8_t harqId = (uint8_t)((*itProcId).second - HARQ_PERIOD) % HARQ_PROC_NUM;
              NS_LOG_INFO (this << " UL-HARQ retx RNTI " << rnti << " harqId " << (uint16_t)harqId << " i " << i << " size "  << params.m_ulInfoList.size ());
              std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itHarq = m_ulHarqProcessesDciBuffer.find (rnti);
              if (itHarq == m_ulHarqProcessesDciBuffer.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                  continue;
                }
              UlDciListElement_s dci = (*itHarq).second.at (harqId);
              std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (rnti);
              if (itStat == m_ulHarqProcessesStatus.end ())
                {
                  NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << rnti);
                }
              if ((*itStat).second.at (harqId) >= 3)
                {
                  NS_LOG_INFO ("Max number of retransmissions reached (UL)-> drop process");
                  continue;
                }
              bool free = true;
              for (int j = dci.m_rbStart; j < dci.m_rbStart + dci.m_rbLen; j++)
                {
                  if (rbMap.at (j) == true)
                    {
                      free = false;
                      NS_LOG_INFO (this << " BUSY " << j);
                    }
                }
              if (free)
                {
                  // retx on the same RBs
                  for (int j = dci.m_rbStart; j < dci.m_rbStart + dci.m_rbLen; j++)
                    {
                      rbMap.at (j) = true;
                      rbgAllocationMap.at (j) = dci.m_rnti;
                      NS_LOG_INFO ("\tRB " << j);
                      rbAllocatedNum++;
                    }
                  NS_LOG_INFO (this << " Send retx in the same RBs " << (uint16_t)dci.m_rbStart << " to " << dci.m_rbStart + dci.m_rbLen << " RV " << (*itStat).second.at (harqId) + 1);
                }
              else
                {
                  NS_LOG_INFO ("Cannot allocate retx due to RACH allocations for UE " << rnti);
                  continue;
                }
              dci.m_ndi = 0;
              // Update HARQ buffers with new HarqId
              (*itStat).second.at ((*itProcId).second) = (*itStat).second.at (harqId) + 1;
              (*itStat).second.at (harqId) = 0;
              (*itHarq).second.at ((*itProcId).second) = dci;
              ret.m_dciList.push_back (dci);
              rntiAllocated.insert (dci.m_rnti);
            }
          else
            {
              NS_LOG_INFO (this << " HARQ-ACK feedback from RNTI " << params.m_ulInfoList.at (i).m_rnti);
            }
        }
    }

  std::map <uint16_t,uint32_t>::iterator it;
  int nflows = 0;

  for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      // select UEs with queues not empty and not yet allocated for HARQ
      if (((*it).second > 0)&&(itRnti == rntiAllocated.end ()))
        {
          nflows++;
        }
    }
  if (nflows == 0)
    {
      if (ret.m_dciList.size () > 0)
        {
          m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
          m_schedSapUser->SchedUlConfigInd (ret);
        }

      return;  // no flows to be scheduled
    }


  // Divide the remaining resources equally among the active users starting from the subsequent one served last scheduling trigger
  uint16_t tempRbPerFlow = (ffrUlBandwidth) / (nflows + rntiAllocated.size ());
  uint16_t rbPerFlow = (minContinuousUlBandwidth < tempRbPerFlow) ? minContinuousUlBandwidth : tempRbPerFlow;

  if (rbPerFlow < 3)
    {
      rbPerFlow = 3;  // at least 3 rbg per flow (till available resource) to ensure TxOpportunity >= 7 bytes
    }

  int rbAllocated = 0;

  std::map <uint16_t, asyncFlowPerf_t>::iterator itStats;
  if (m_nextRntiUl != 0)
    {
      for (it = m_ceBsrRxed.begin (); it != m_ceBsrRxed.end (); it++)
        {
          if ((*it).first == m_nextRntiUl)
            {
              break;
            }
        }
      if (it == m_ceBsrRxed.end ())
        {
          NS_LOG_ERROR (this << " no user found");
        }
    }
  else
    {
      it = m_ceBsrRxed.begin ();
      m_nextRntiUl = (*it).first;
    }
  do
    {
      std::set <uint16_t>::iterator itRnti = rntiAllocated.find ((*it).first);
      if ((itRnti != rntiAllocated.end ())||((*it).second == 0))
        {
          // UE already allocated for UL-HARQ -> skip it
          NS_LOG_DEBUG (this << " UE already allocated in HARQ -> discared, RNTI " << (*it).first);
          it++;
          if (it == m_ceBsrRxed.end ())
            {
              // restart from the first
              it = m_ceBsrRxed.begin ();
            }
          continue;
        }
      if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
        {
          // limit to physical resources last resource assignment
          rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
          // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
          if (rbPerFlow < 3)
            {
              // terminate allocation
              rbPerFlow = 0;
            }
        }

      rbAllocated = 0;
      UlDciListElement_s uldci;
      uldci.m_rnti = (*it).first;
      uldci.m_rbLen = rbPerFlow;
      bool allocated = false;

      while ((!allocated)&&((rbAllocated + rbPerFlow - m_cschedCellConfig.m_ulBandwidth) < 1) && (rbPerFlow != 0))
        {
          // check availability
          bool free = true;
          for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
            {
              if (rbMap.at (j) == true)
                {
                  free = false;
                  break;
                }
              if ((m_ffrSapProvider->IsUlRbgAvailableForUe (j, (*it).first)) == false)
                {
                  free = false;
                  break;
                }
            }
          if (free)
            {
              NS_LOG_INFO (this << "RNTI: "<< (*it).first<< " RB Allocated " << rbAllocated << " rbPerFlow " << rbPerFlow << " flows " << nflows);
              uldci.m_rbStart = rbAllocated;

              for (uint16_t j = rbAllocated; j < rbAllocated + rbPerFlow; j++)
                {
                  rbMap.at (j) = true;
                  // store info on allocation for managing ul-cqi interpretation
                  rbgAllocationMap.at (j) = (*it).first;
                }
              rbAllocated += rbPerFlow;
              allocated = true;
              break;
            }
          rbAllocated++;
          if (rbAllocated + rbPerFlow - 1 > m_cschedCellConfig.m_ulBandwidth)
            {
              // limit to physical resources last resource assignment
              rbPerFlow = m_cschedCellConfig.m_ulBandwidth - rbAllocated;
              // at least 3 rbg per flow to ensure TxOpportunity >= 7 bytes
              if (rbPerFlow < 3)
                {
                  // terminate allocation
                  rbPerFlow = 0;
                }
            }
        }
      if (!allocated)
        {
          // unable to allocate new resource: finish scheduling
          m_nextRntiUl = (*it).first;
//          if (ret.m_dciList.size () > 0)
//            {
//              m_schedSapUser->SchedUlConfigInd (ret);
//            }
//          m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
//          return;
          break;
        }



      std::map <uint16_t, std::vector <double> >::iterator itCqi = m_ueCqi.find ((*it).first);
      int cqi = 0;
      if (itCqi == m_ueCqi.end ())
        {
          // no cqi info about this UE
          uldci.m_mcs = 0; // MCS 0 -> UL-AMC TBD
        }
      else
        {
          // take the lowest CQI value (worst RB)
          NS_ABORT_MSG_IF ((*itCqi).second.size() == 0, "CQI of RNTI = " << (*it).first << " has expired");
          double minSinr = (*itCqi).second.at (uldci.m_rbStart);
          if (minSinr == NO_SINR)
            {
              minSinr = EstimateUlSinr ((*it).first, uldci.m_rbStart);
            }
          for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
            {
              double sinr = (*itCqi).second.at (i);
              if (sinr == NO_SINR)
                {
                  sinr = EstimateUlSinr ((*it).first, i);
                }
              if (sinr < minSinr)
                {
                  minSinr = sinr;
                }
            }

          // translate SINR -> cqi: WILD ACK: same as DL
          double s = log2 ( 1 + (
                              std::pow (10, minSinr / 10 )  /
                              ( (-std::log (5.0 * 0.00005 )) / 1.5) ));
          cqi = m_amc->GetCqiFromSpectralEfficiency (s);
          if (cqi == 0)
            {
              it++;
              if (it == m_ceBsrRxed.end ())
                {
                  // restart from the first
                  it = m_ceBsrRxed.begin ();
                }
              NS_LOG_DEBUG (this << " UE discarded for CQI = 0, RNTI " << uldci.m_rnti);
              // remove UE from allocation map
              for (uint16_t i = uldci.m_rbStart; i < uldci.m_rbStart + uldci.m_rbLen; i++)
                {
                  rbgAllocationMap.at (i) = 0;
                }
              continue; // CQI == 0 means "out of range" (see table 7.2.3-1 of 36.213)
            }
          uldci.m_mcs = m_amc->GetMcsFromCqi (cqi);
        }

      uldci.m_tbSize = (m_amc->GetUlTbSizeFromMcs (uldci.m_mcs, rbPerFlow) / 8);
      UpdateUlRlcBufferInfo (uldci.m_rnti, uldci.m_tbSize);
      uldci.m_ndi = 1;
      uldci.m_cceIndex = 0;
      uldci.m_aggrLevel = 1;
      uldci.m_ueTxAntennaSelection = 3; // antenna selection OFF
      uldci.m_hopping = false;
      uldci.m_n2Dmrs = 0;
      uldci.m_tpc = 0; // no power control
      uldci.m_cqiRequest = false; // only period CQI at this stage
      uldci.m_ulIndex = 0; // TDD parameter
      uldci.m_dai = 1; // TDD parameter
      uldci.m_freqHopping = 0;
      uldci.m_pdcchPowerOffset = 0; // not used
      ret.m_dciList.push_back (uldci);
      // store DCI for HARQ_PERIOD
      uint8_t harqId = 0;
      if (m_harqOn == true)
        {
          std::map <uint16_t, uint8_t>::iterator itProcId;
          itProcId = m_ulHarqCurrentProcessId.find (uldci.m_rnti);
          if (itProcId == m_ulHarqCurrentProcessId.end ())
            {
              NS_FATAL_ERROR ("No info find in HARQ buffer for UE " << uldci.m_rnti);
            }
          harqId = (*itProcId).second;
          std::map <uint16_t, UlHarqProcessesDciBuffer_t>::iterator itDci = m_ulHarqProcessesDciBuffer.find (uldci.m_rnti);
          if (itDci == m_ulHarqProcessesDciBuffer.end ())
            {
              NS_FATAL_ERROR ("Unable to find RNTI entry in UL DCI HARQ buffer for RNTI " << uldci.m_rnti);
            }
          (*itDci).second.at (harqId) = uldci;
          // Update HARQ process status (RV 0)
          std::map <uint16_t, UlHarqProcessesStatus_t>::iterator itStat = m_ulHarqProcessesStatus.find (uldci.m_rnti);
          if (itStat == m_ulHarqProcessesStatus.end ())
            {
              NS_LOG_ERROR ("No info find in HARQ buffer for UE (might change eNB) " << uldci.m_rnti);
            }
          (*itStat).second.at (harqId) = 0;
        }

      NS_LOG_INFO (this << " UE Allocation RNTI " << (*it).first << " startPRB " << (uint32_t)uldci.m_rbStart << " nPRB " << (uint32_t)uldci.m_rbLen << " CQI " << cqi << " MCS " << (uint32_t)uldci.m_mcs << " TBsize " << uldci.m_tbSize << " RbAlloc " << rbAllocated << " harqId " << (uint16_t)harqId);

      // update TTI  UE stats
      itStats = m_flowStatsUl.find ((*it).first);
      if (itStats != m_flowStatsUl.end ())
        {
          (*itStats).second.lastTtiBytesTrasmitted =  uldci.m_tbSize;
        }
      else
        {
          NS_LOG_DEBUG (this << " No Stats for this allocated UE");
        }


      it++;
      if (it == m_ceBsrRxed.end ())
        {
          // restart from the first
          it = m_ceBsrRxed.begin ();
        }
      if ((rbAllocated == m_cschedCellConfig.m_ulBandwidth) || (rbPerFlow == 0))
        {
          // Stop allocation: no more PRBs
          m_nextRntiUl = (*it).first;
          break;
        }
    }
  while (((*it).first != m_nextRntiUl)&&(rbPerFlow!=0));


  // Update global UE stats
  // update UEs stats
  for (itStats = m_flowStatsUl.begin (); itStats != m_flowStatsUl.end (); itStats++)
    {
      (*itStats).second.totalBytesTransmitted += (*itStats).second.lastTtiBytesTrasmitted;
      // update average throughput (see eq. 12.3 of Sec 12.3.1.2 of LTE – The UMTS Long Term Evolution, Ed Wiley)
      (*itStats).second.lastAveragedThroughput = ((1.0 - (1.0 / m_timeWindow)) * (*itStats).second.lastAveragedThroughput) + ((1.0 / m_timeWindow) * (double)((*itStats).second.lastTtiBytesTrasmitted / 0.001));
      NS_LOG_INFO (this << " UE total bytes " << (*itStats).second.totalBytesTransmitted);
      NS_LOG_INFO (this << " UE average throughput " << (*itStats).second.lastAveragedThroughput);
      (*itStats).second.lastTtiBytesTrasmitted = 0;
    }
  m_allocationMaps.insert (std::pair <uint16_t, std::vector <uint16_t> > (params.m_sfnSf, rbgAllocationMap));
  m_schedSapUser->SchedUlConfigInd (ret);

  return;
}

void
AsyncFfMacScheduler::DoSchedUlNoiseInterferenceReq (const struct FfMacSchedSapProvider::SchedUlNoiseInterferenceReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  return;
}

void
AsyncFfMacScheduler::DoSchedUlSrInfoReq (const struct FfMacSchedSapProvider::SchedUlSrInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  return;
}

void
AsyncFfMacScheduler::DoSchedUlMacCtrlInfoReq (const struct FfMacSchedSapProvider::SchedUlMacCtrlInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);

  std::map <uint16_t,uint32_t>::iterator it;

  for (unsigned int i = 0; i < params.m_macCeList.size (); i++)
    {
      if ( params.m_macCeList.at (i).m_macCeType == MacCeListElement_s::BSR )
        {
          // buffer status report
          // note that this scheduler does not differentiate the
          // allocation according to which LCGs have more/less bytes
          // to send.
          // Hence the BSR of different LCGs are just summed up to get
          // a total queue size that is used for allocation purposes.

          uint32_t buffer = 0;
          for (uint8_t lcg = 0; lcg < 4; ++lcg)
            {
              uint8_t bsrId = params.m_macCeList.at (i).m_macCeValue.m_bufferStatus.at (lcg);
              buffer += BufferSizeLevelBsr::BsrId2BufferSize (bsrId);
            }

          uint16_t rnti = params.m_macCeList.at (i).m_rnti;
          NS_LOG_LOGIC (this << "RNTI=" << rnti << " buffer=" << buffer);
          it = m_ceBsrRxed.find (rnti);
          if (it == m_ceBsrRxed.end ())
            {
              // create the new entry
              m_ceBsrRxed.insert ( std::pair<uint16_t, uint32_t > (rnti, buffer));
            }
          else
            {
              // update the buffer size value
              (*it).second = buffer;
            }
        }
    }

  return;
}

void
AsyncFfMacScheduler::DoSchedUlCqiInfoReq (const struct FfMacSchedSapProvider::SchedUlCqiInfoReqParameters& params)
{
  NS_LOG_FUNCTION (this);
  m_ffrSapProvider->ReportUlCqiInfo (params);

// retrieve the allocation for this subframe
  switch (m_ulCqiFilter)
    {
    case FfMacScheduler::SRS_UL_CQI:
      {
        // filter all the CQIs that are not SRS based
        if (params.m_ulCqi.m_type != UlCqi_s::SRS)
          {
            return;
          }
      }
      break;
    case FfMacScheduler::PUSCH_UL_CQI:
      {
        // filter all the CQIs that are not SRS based
        if (params.m_ulCqi.m_type != UlCqi_s::PUSCH)
          {
            return;
          }
      }
      break;

    default:
      NS_FATAL_ERROR ("Unknown UL CQI type");
    }

  switch (params.m_ulCqi.m_type)
    {
    case UlCqi_s::PUSCH:
      {
        std::map <uint16_t, std::vector <uint16_t> >::iterator itMap;
        std::map <uint16_t, std::vector <double> >::iterator itCqi;
        NS_LOG_DEBUG (this << " Collect PUSCH CQIs of Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
        itMap = m_allocationMaps.find (params.m_sfnSf);
        if (itMap == m_allocationMaps.end ())
          {
            return;
          }
        for (uint32_t i = 0; i < (*itMap).second.size (); i++)
          {
            // convert from fixed point notation Sxxxxxxxxxxx.xxx to double
            double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (i));
            itCqi = m_ueCqi.find ((*itMap).second.at (i));
            if (itCqi == m_ueCqi.end ())
              {
                // create a new entry
                std::vector <double> newCqi;
                for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
                  {
                    if (i == j)
                      {
                        newCqi.push_back (sinr);
                      }
                    else
                      {
                        // initialize with NO_SINR value.
                        newCqi.push_back (NO_SINR);
                      }

                  }
                m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > ((*itMap).second.at (i), newCqi));
                // generate correspondent timer
                m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > ((*itMap).second.at (i), m_cqiTimersThreshold));
              }
            else
              {
                // update the value
                (*itCqi).second.at (i) = sinr;
                NS_LOG_DEBUG (this << " RNTI " << (*itMap).second.at (i) << " RB " << i << " SINR " << sinr);
                // update correspondent timer
                std::map <uint16_t, uint32_t>::iterator itTimers;
                itTimers = m_ueCqiTimers.find ((*itMap).second.at (i));
                (*itTimers).second = m_cqiTimersThreshold;

              }

          }
        // remove obsolete info on allocation
        m_allocationMaps.erase (itMap);
      }
      break;
    case UlCqi_s::SRS:
      {
    	 NS_LOG_DEBUG (this << " Collect SRS CQIs of Frame no. " << (params.m_sfnSf >> 4) << " subframe no. " << (0xF & params.m_sfnSf));
        // get the RNTI from vendor specific parameters
        uint16_t rnti = 0;
        NS_ASSERT (params.m_vendorSpecificList.size () > 0);
        for (uint16_t i = 0; i < params.m_vendorSpecificList.size (); i++)
          {
            if (params.m_vendorSpecificList.at (i).m_type == SRS_CQI_RNTI_VSP)
              {
                Ptr<SrsCqiRntiVsp> vsp = DynamicCast<SrsCqiRntiVsp> (params.m_vendorSpecificList.at (i).m_value);
                rnti = vsp->GetRnti ();
              }
          }
        std::map <uint16_t, std::vector <double> >::iterator itCqi;
        itCqi = m_ueCqi.find (rnti);
        if (itCqi == m_ueCqi.end ())
          {
            // create a new entry
            std::vector <double> newCqi;
            for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
              {
                double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
                newCqi.push_back (sinr);
                NS_LOG_INFO (this << " RNTI " << rnti << " new SRS-CQI for RB  " << j << " value " << sinr);

              }
            m_ueCqi.insert (std::pair <uint16_t, std::vector <double> > (rnti, newCqi));
            // generate correspondent timer
            m_ueCqiTimers.insert (std::pair <uint16_t, uint32_t > (rnti, m_cqiTimersThreshold));
          }
        else
          {
            // update the values
            for (uint32_t j = 0; j < m_cschedCellConfig.m_ulBandwidth; j++)
              {
                double sinr = LteFfConverter::fpS11dot3toDouble (params.m_ulCqi.m_sinr.at (j));
                (*itCqi).second.at (j) = sinr;
                NS_LOG_INFO (this << " RNTI " << rnti << " update SRS-CQI for RB  " << j << " value " << sinr);
              }
            // update correspondent timer
            std::map <uint16_t, uint32_t>::iterator itTimers;
            itTimers = m_ueCqiTimers.find (rnti);
            (*itTimers).second = m_cqiTimersThreshold;

          }


      }
      break;
    case UlCqi_s::PUCCH_1:
    case UlCqi_s::PUCCH_2:
    case UlCqi_s::PRACH:
      {
        NS_FATAL_ERROR ("AsyncFfMacScheduler supports only PUSCH and SRS UL-CQIs");
      }
      break;
    default:
      NS_FATAL_ERROR ("Unknown type of UL-CQI");
    }
  return;
}

void
AsyncFfMacScheduler::RefreshDlCqiMaps (void)
{
  // refresh DL CQI P01 Map
  std::map <uint16_t,uint32_t>::iterator itP10 = m_p10CqiTimers.begin ();
  while (itP10 != m_p10CqiTimers.end ()) {
    NS_LOG_INFO (this << " P10-CQI for user " << (*itP10).first << " is " << (uint32_t)(*itP10).second << " thr " << (uint32_t)m_cqiTimersThreshold);
    if ((*itP10).second == 0) {
      // delete correspondent entries
      std::map <uint16_t,uint8_t>::iterator itMap = m_p10CqiRxed.find ((*itP10).first);
      NS_ASSERT_MSG (itMap != m_p10CqiRxed.end (), " Does not find CQI report for user " << (*itP10).first);
      NS_LOG_INFO (this << " P10-CQI expired for user " << (*itP10).first);
      m_p10CqiRxed.erase (itMap);
      std::map <uint16_t,uint32_t>::iterator temp = itP10;
      itP10++;
      m_p10CqiTimers.erase (temp);
    }
    else {
      (*itP10).second--;
      itP10++;
    }
  }
 
  // refresh DL CQI A30 Map
  std::map <uint16_t,uint32_t>::iterator itA30 = m_a30CqiTimers.begin ();
  while (itA30 != m_a30CqiTimers.end ()) {
    NS_LOG_INFO (this << " A30-CQI for user " << (*itA30).first << " is " << (uint32_t)(*itA30).second << " thr " << (uint32_t)m_cqiTimersThreshold);
    if ((*itA30).second == 0) {
      // delete correspondent entries
      std::map <uint16_t,SbMeasResult_s>::iterator itMap = m_a30CqiRxed.find ((*itA30).first);
      NS_ASSERT_MSG (itMap != m_a30CqiRxed.end (), " Does not find CQI report for user " << (*itA30).first);
      NS_LOG_INFO (this << " A30-CQI expired for user " << (*itA30).first);
      m_a30CqiRxed.erase (itMap);
      std::map <uint16_t,uint32_t>::iterator temp = itA30;
      itA30++;
      m_a30CqiTimers.erase (temp);
    }
    else {
      (*itA30).second--;
      itA30++;
    }
  }
 
  return;
}


void
AsyncFfMacScheduler::RefreshUlCqiMaps (void)
{
  // refresh UL CQI  Map
  std::map <uint16_t,uint32_t>::iterator itUl = m_ueCqiTimers.begin ();
  while (itUl != m_ueCqiTimers.end ())
    {
      NS_LOG_INFO (this << " UL-CQI for user " << (*itUl).first << " is " << (uint32_t)(*itUl).second << " thr " << (uint32_t)m_cqiTimersThreshold);
      if ((*itUl).second == 0)
        {
          // delete correspondent entries
          std::map <uint16_t, std::vector <double> >::iterator itMap = m_ueCqi.find ((*itUl).first);
          NS_ASSERT_MSG (itMap != m_ueCqi.end (), " Does not find CQI report for user " << (*itUl).first);
          NS_LOG_INFO (this << " UL-CQI exired for user " << (*itUl).first);
          (*itMap).second.clear ();
          m_ueCqi.erase (itMap);
          std::map <uint16_t,uint32_t>::iterator temp = itUl;
          itUl++;
          m_ueCqiTimers.erase (temp);
        }
      else
        {
          (*itUl).second--;
          itUl++;
        }
    }

  return;
}

void
AsyncFfMacScheduler::UpdateDlRlcBufferInfo (uint16_t rnti, uint8_t lcid, uint16_t size)
{
  LteFlowId_t flow (rnti, lcid);

  if(m_rlcBufferReq.count(flow) == 0) {
    NS_LOG_ERROR (this << " Does not find DL RLC Buffer Report of UE " << rnti);
    return;
  }

  FfMacSchedSapProvider::SchedDlRlcBufferReqParameters& rrp = m_rlcBufferReq[flow];
  NS_LOG_INFO (this << " UE " << rnti << " LC " << (uint16_t)lcid << " txqueue " << rrp.m_rlcTransmissionQueueSize 
                    << " retxqueue " << rrp.m_rlcRetransmissionQueueSize 
                    << " status " << rrp.m_rlcStatusPduSize << " decrease " << size);
  // Update queues: RLC tx order Status, ReTx, Tx
  // Update status queue
  if ((rrp.m_rlcStatusPduSize > 0) && (size >= rrp.m_rlcStatusPduSize)) {
    rrp.m_rlcStatusPduSize = 0;
  }
  else if ((rrp.m_rlcRetransmissionQueueSize > 0) && (size >= rrp.m_rlcRetransmissionQueueSize)) {
    rrp.m_rlcRetransmissionQueueSize = 0;
  }
  else if (rrp.m_rlcTransmissionQueueSize > 0) {
    uint32_t rlcOverhead;
    if (lcid == 1) {
      // for SRB1 (using RLC AM) it's better to overestimate RLC overhead rather than
      // underestimate it and risk unneeded segmentation which increases delay 
      rlcOverhead = 4;
    }
    else {
      // minimum RLC overhead due to header
      rlcOverhead = 2;
    }

    // update transmission queue
    if (rrp.m_rlcTransmissionQueueSize <= size - rlcOverhead) {
      rrp.m_rlcTransmissionQueueSize = 0;
    }
    else {
      rrp.m_rlcTransmissionQueueSize -= size - rlcOverhead;
    }
  }
}

void
AsyncFfMacScheduler::UpdateUlRlcBufferInfo (uint16_t rnti, uint16_t size)
{

  size = size - 2; // remove the minimum RLC overhead
  std::map <uint16_t,uint32_t>::iterator it = m_ceBsrRxed.find (rnti);
  if (it != m_ceBsrRxed.end ())
    {
      NS_LOG_INFO (this << " UE " << rnti << " size " << size << " BSR " << (*it).second);
      if ((*it).second >= size)
        {
          (*it).second -= size;
        }
      else
        {
          (*it).second = 0;
        }
    }
  else
    {
      NS_LOG_ERROR (this << " Does not find BSR report info of UE " << rnti);
    }

}

void
AsyncFfMacScheduler::TransmissionModeConfigurationUpdate (uint16_t rnti, uint8_t txMode)
{
  NS_LOG_FUNCTION (this << " RNTI " << rnti << " txMode " << (uint16_t)txMode);
  FfMacCschedSapUser::CschedUeConfigUpdateIndParameters params;
  params.m_rnti = rnti;
  params.m_transmissionMode = txMode;
  m_cschedSapUser->CschedUeConfigUpdateInd (params);
}


}
