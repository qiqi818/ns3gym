#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import gym
# import matplotlib.pyplot as plt
import numpy as np
from policy_gradient import PolicyGradient
import random
import argparse
from ns3gym import ns3env
from gym import spaces
import matplotlib.pyplot as plt
__author__ = "Piotr Gawlowicz"
__copyright__ = "Copyright (c) 2018, Technische Universität Berlin"
__version__ = "0.1.0"
__email__ = "gawlowicz@tkn.tu-berlin.de"


parser = argparse.ArgumentParser(description='Start simulation script on/off')
parser.add_argument('--start',
                    type=int,
                    default=1,
                    help='Start ns-3 simulation script 0/1, Default: 1')
parser.add_argument('--iterations',
                    type=int,
                    default=1,
                    help='Number of iterations, Default: 1')
args = parser.parse_args()
startSim = bool(args.start)
iterationNum = int(args.iterations)

port = 5555
simTime = 10 # seconds
stepTime = 0.001  # seconds
seed = 0
simArgs = {"--simTime": simTime,
           "--testArg": 123}
debug = False

#------------------------------------------------------------------------------------------------
#/home/liqi/.local/lib/python3.6/site-packages/gym/envs/
# env1 = gym.make('sateDCA_ENV-v0')
# env1 = env1.unwrapped
nOfenb = 2
nOfchannel = 12
nOfue = 2
sizeperq = 4#每个有效请求的长度

if __name__ == "__main__":

    # Load checkpoint
    load_path = None 
    save_path = None 

    PG = PolicyGradient(n_x = sizeperq+nOfenb*nOfchannel,n_y = nOfchannel*nOfenb,learning_rate=0.005,reward_decay=1,load_path=load_path,save_path=save_path,ep=0.99,nOfChannel = nOfchannel)

env = ns3env.Ns3Env(port=port, startSim=startSim, simSeed=seed, simArgs=simArgs, debug=debug)


env.reset()

ob_space = env.observation_space
ac_space = env.action_space

stepIdx = 0
currIt = 0

# ------------------------------------------------------------------------------------------------
ax = []
ay = []
stepIdx = -1
currIt = 0
rd = []
plt.ion()
try:
    while True:
        print("Start iteration: ", currIt)
        obs = env.reset()
        print("Step: ", stepIdx)
        print("---obs:", obs)
        flag = False
        while True:
            reward = 0
            matrixOfChanAlloc = np.zeros((nOfenb, nOfchannel))
            
            stepIdx += 1
            if stepIdx % 100 == 0:
                PG.ep = PG.ep * 0.7
            ax.append(stepIdx)
            print("stepIdx: ",stepIdx)
            # ax.append(stepIdx)
            # ---------------------------------------------------------------------------------------
            observation = []#环境的观测值，状态observation
            numue = 0
            
            for j in range(int(len(obs)/sizeperq)):
                #状态
                observation.append([obs[sizeperq*j],obs[sizeperq*j+1],obs[sizeperq*j+2],obs[sizeperq*j+3]])
                if obs[sizeperq*j+1] != 0: #若rnti不为0则为有效请求
                    numue += 1          #统计有效请求数
            
            action_list = []#存储动作的list
            print("obs: ",obs)
            if numue == 0:
                action_list.append(0)
                action_list.append(0)
                action_list.append(0)
                d = ()
                for b in range(len(action_list)):
                    d += (spaces.Discrete(int(action_list[b])),)
                action_ = spaces.Tuple(d)
                obs, reward_step, done, info = env.step(action_)#获取这一eposide的奖励
                ay.append(reward_step+0.5)
            else:
                for k in range(numue):

                    ss = []
                    # for a in observation:
                    #     for b in a:
                    #         ss.append(b)
                    ss = observation[k].copy()
                    ss.extend(matrixOfChanAlloc.copy().reshape(1,nOfenb*nOfchannel).tolist()[0])#请求+信道占用 

                    observation_step = np.array(ss).reshape(nOfenb*nOfchannel+sizeperq,1).ravel()#变换为网络输入所要求的维度
                    # print("observation_step: ",observation_step)
                    if observation_step[1] > 0:#判断RNTI是否大于0 是否为有效请求
                        action = PG.choose_action1(observation_step,matrixOfChanAlloc,observation_step[0])#选取动作
                        
                        if action < nOfchannel:         #判断是否为有效动作
                            action_list.append(observation_step[0])#cellid
                            action_list.append(observation_step[1])#rnti
                            action_list.append(action)  #资源编号
                        else:
                            action_list.append(0)
                            action_list.append(0)
                            action_list.append(0)
                    reward = 0
                    if stepIdx > 100 and k < numue-1:
                        # s,a,r = PG.store_transition(observation_step, action+nOfchannel*int(observation_step[0]), reward)
                        s,a,r = PG.store_transition(observation_step, action, reward)
                #大step
                d = ()
                for b in range(len(action_list)):
                    d += (spaces.Discrete(int(action_list[b])),)
                action_ = spaces.Tuple(d)
                obs, reward_step, done, info = env.step(action_)#获取这一eposide的奖励
                ay.append(reward_step+0.5)
                plt.clf()              # 清除之前画的图
                plt.plot(ax,ay)        # 画出当前 ax 列表和 ay 列表中的值的图形
                plt.xlabel('step')
                plt.ylabel('吞吐率')
                plt.pause(0.01)         # 暂停
                plt.ioff()             # 关闭画图的窗口
                reward = reward_step
                if stepIdx > 100:
                    # s,a,r = PG.store_transition(observation_step, action+int(nOfchannel*observation[numue-1][0]), reward)
                    s,a,r = PG.store_transition(observation_step, action, reward)

                if flag == False and (stepIdx-100)%4 == 0 and stepIdx > 100:
                    PG.learn()
                if flag:
                    PG.learn()
                
                
        currIt += 1
        if currIt == iterationNum:
            break

except KeyboardInterrupt:
    print("Ctrl-C -> Exit")
finally:
    print(len(ax))
    print(len(ay))
    plt.plot(ax[0:len(ax)-1],ay)
    # env.close()
    print("Done")

