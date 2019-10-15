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
stepTime = 0.5  # seconds
seed = 0
simArgs = {"--simTime": simTime,
           "--testArg": 123}
debug = False

#------------------------------------------------------------------------------------------------
#/home/liqi/.local/lib/python3.6/site-packages/gym/envs/
env1 = gym.make('sateDCA_ENV-v0')
env1 = env1.unwrapped



if __name__ == "__main__":

    # Load checkpoint
    load_path = None 
    save_path = None 

    PG = PolicyGradient(n_x = 7*12,n_y = 7*12,learning_rate=0.0005,reward_decay=0.95,load_path=load_path,save_path=save_path)

env = ns3env.Ns3Env(port=port, startSim=startSim, simSeed=seed, simArgs=simArgs, debug=debug)


env.reset()

ob_space = env.observation_space
ac_space = env.action_space


stepIdx = 0
currIt = 0

try:
    while True:
        print("Start iteration: ", currIt)
        obs = env.reset()
        # print("Step: ", stepIdx)
        print("---obs:", obs)

        while True:
            stepIdx += 1

            #---------------------------------------------------------------------------------------
            rewards = []
            EPISODES = 100                     
            env1.reset()
            for i in range(EPISODES):#-------------------------------------------------------学习片段
                matrixOfChanAlloc = np.zeros((7, 12))         #信道占用矩阵
                channel_alloc = matrixOfChanAlloc.copy()
                env1.beamChanAlloc = channel_alloc.copy()
                
                env1.beamOfUser = np.array([1, 1 ,1 ,1, 1, 1, 1])#obs
                
                
                
                # if obs is None or obs.__len__ == 0:
                #     #continue
                #     env1.beamOfUser = np.array([0, 0 ,0 ,0, 0,0, 0])
                # else:
                #     for j in range(7):
                #     #     #env1.beamOfUser[j] = (int)(obs[j])#观测状态 每个波束下的用户数
                #         env1.beamOfUser[j] = (int)(obs[j])#观测状态 每个波束下的用户数
            
                sumu = sum(env1.beamOfUser)
                if (env1.beamOfUser==0).all():
                    continue
                observation = env1.beamChanAlloc
                episode_reward = 0

                while True:

            
                    # 1. Choose an action based on observation
            
            
                    observation = observation.copy().reshape(7*12,1).ravel()
            
                    action,flag = PG.choose_action(observation,env1)
            
                    if flag:
            
                        episode_rewards_sum = sum(PG.episode_rewards)
                        rewards.append(episode_rewards_sum)
                        max_reward_so_far = np.amax(rewards)
            
            
                        # 4. Train neural network
                        discounted_episode_rewards_norm = PG.learn()
            
                        
                        
                        break
            
                    # 2. Take action in the environment
                    observation_, reward, done, info = env1.step(action)
            
                    
                    # 3. Store transition for training
                    s,a,r = PG.store_transition(observation, action, reward)
            
                    if done and (not flag):
                        episode_rewards_sum = sum(PG.episode_rewards)
                        rewards.append(episode_rewards_sum)
                        max_reward_so_far = np.amax(rewards)
            
                        # 4. Train neural network
                        discounted_episode_rewards_norm = PG.learn()
            
                        # Render env if we get to rewards minimum
            
                        break
            
                    # Save new observation
                    observation = observation_                    
            
            if(sumu == 0):
                action_list=[9999]
                d = (spaces.Discrete(9999),)
                action = spaces.Tuple(d)
            else:
                d = ()
                action_list = []            
                for i in range(len(a)):
                    d+=(spaces.Discrete(np.where(a[i]==1)[0][0]),)
                    action_list.append(np.where(a[i]==1)[0][0])
                action = spaces.Tuple(d)
            
            #-----------------------------------------------------------------------------
            
            
            #---------------------------------------------------------------------------------------
            print("---action: ", action_list)
            print("---------------------------------------------")
            # print("Step: ", stepIdx)
            obs, reward, done, info = env.step(action)
            print("---obs: ", obs)
            print("---reward: ", reward)

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
    env.close()
    print("Done")
