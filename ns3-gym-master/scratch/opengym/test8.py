#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import gym

import numpy as np
from rnn import ResourceAllocationRNN
import random
import argparse
from ns3gym import ns3env
from gym import spaces
from rnn import TRNNConfig, ResourceAllocationRNN
import tensorflow as tf
import matplotlib.pyplot as plt
# __author__ = "Piotr Gawlowicz"
# __copyright__ = "Copyright (c) 2018, Technische Universität Berlin"
# __version__ = "0.1.0"
# __email__ = "gawlowicz@tkn.tu-berlin.de"

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
simTime = 200  # seconds
stepTime = 0.5  # seconds
seed = 0
simArgs = {"--simTime": simTime,
           "--testArg": 123}
debug = False

# ------------------------------------------------------------------------------------------------

if __name__ == "__main__":
    # Load checkpoint
    load_path = None
    save_path = None
    #创建RNN网络
    config = TRNNConfig()
    RNN = ResourceAllocationRNN(config)
#/home/liqi/.local/lib/python3.6/site-packages/ns3gym
#初始化环境env
env = ns3env.Ns3Env(port=port, stepTime=stepTime, startSim=startSim, simSeed=seed, simArgs=simArgs, debug=debug)
env.reset()

ob_space = env.observation_space
ac_space = env.action_space
ax = []
ay = []
stepIdx = -1
currIt = 0
plt.ion()

	
try:
    while True:
        print("Start iteration: ", currIt)
        obs = env.reset()
        print("Step: ", stepIdx)
        print("---obs:", obs)

        while True:
            stepIdx += 1
            ax.append(stepIdx) 
            # ---------------------------------------------------------------------------------------
            observation = []#环境的观测值，状态observation
            #状态是最长为84的[CellId,RNTI]组合，不足84则补零
            if len(obs)<4800:
                for kk in range(4800 - len(obs)):
                    obs.append(0)
            for j in range(1200):
                #状态是最长为84的[CellId,RNTI]组合
                observation.append([obs[4*j],obs[4*j+1],obs[4*j+2],obs[4*j+3]])

            observation_step = [observation] 
            
            
            # 将状态输入RNN选取动作  
            # if stepIdx < 1:
            #     action_step = RNN.choose_action(observation_step)
            # print(observation_step)
            # if stepIdx < 500:
            #     tj = np.zeros([7, 12])
            #     action_step = np.zeros([7, 12])
            #     for q in range(7):
            #         id = 0
            #         for p in range(len(observation_step[0])):
            #             if observation_step[0][p][0] == q and observation_step[0][p][1] > 0:
            #                 action_step[q][id] = observation_step[0][p][1]
            #                 tj[q][id]+=1
            #                 id += 1
            #                 if id == 12:
            #                     break
               
            # else:
            action_step,tj = RNN.choose_action2(observation_step)
            print(action_step)
            print(tj)
            # r = 0
            # for i in range(7):
            #     for j in range(12):
            #         if tj[i][j] == 1:
            #             r += 100
            #         if tj[i][j] > 1:
            #             r -= ((tj[i][j]-1) * 10)
            action_list = []
            #将动作以[CellId1,RNTI1,资源编号1,CellId2,RNTI2,资源编号2,……]的形式保存在action_list中
            for x in range(7):
                for y in range(12):
                    if action_step[x][y] > 0:
                        action_list.append(x)                       #CellId
                        action_list.append(int(action_step[x][y]))  #RNTI
                        action_list.append(y)                       #资源编号
            #将动作封装到spaces.Tuple类型中，以备传送给ns3
            d = ()
            for b in range(len(action_list)):
                d += (spaces.Discrete(int(action_list[b])),)
            action_ = spaces.Tuple(d)
        
            print("Step: ", stepIdx)
            print("---obs:\n ", obs)
            print("---action:\n ", action_list)
            
            #step函数，将动作action_传给ns3中执行下去，并获得ns3中的反馈
            obs, reward_step, done, info = env.step(action_)
            # reward_step  = r
            ay.append(reward_step)
            print("---reward: ", reward_step)
            print("-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-")
            #将每一步的状态、动作、奖励存储，以备进行学习
            s, a, r = RNN.store_transition(observation_step, action_step, reward_step)

            plt.clf()              # 清除之前画的图
            plt.plot(ax,ay)        # 画出当前 ax 列表和 ay 列表中的值的图形
            plt.xlabel('step')
            plt.ylabel('reward')
            plt.pause(0.1)         # 暂停一秒
            plt.ioff()             # 关闭画图的窗口
            
            #每经历10个step，网络进行一次学习
            if  stepIdx > 0 and (stepIdx+1)%10 == 0:
                discounted_episode_rewards_norm = RNN.learn()


            if done:
                stepIdx = 0
                if currIt + 1 < iterationNum:
                    env.reset()
                break

        currIt += 1
        if currIt == iterationNum:
            break

except KeyboardInterrupt:
    print("Ctrl-C -> Exit")
finally:
    # env.close()
    print("Done")

