"""
Policy Gradient Reinforcement Learning
Uses a 3 layer neural network as the policy network

"""
import tensorflow as tf
import numpy as np
from tensorflow.python.framework import ops
import random

class PolicyGradient:
    def __init__(
        self,
        n_x,
        n_y,
        learning_rate=0.05,
        reward_decay=0.95,
        load_path=None,
        save_path=None,
        ep = 0.99,
        nOfChannel = 0
    ):

        self.n_x = n_x
        self.n_y = n_y
        self.lr = learning_rate
        self.gamma = reward_decay

        self.save_path = None
        if save_path is not None:
            self.save_path = save_path

        self.episode_observations, self.episode_actions, self.episode_rewards = [], [], []

        self.build_network()

        self.cost_history = []
        self.ep = ep
        self.nOfChannel = nOfChannel
        self.sess = tf.Session()

        # $ tensorboard --logdir=logs
        # http://0.0.0.0:6006/
        tf.summary.FileWriter("logs/", self.sess.graph)

        self.sess.run(tf.global_variables_initializer())

        # 'Saver' op to save and restore all the variables
        self.saver = tf.train.Saver()

        # Restore model
        if load_path is not None:
            self.load_path = load_path
            self.saver.restore(self.sess, self.load_path)

    def store_transition(self, s, a, r):
        """
            Store play memory for training

            Arguments:
                s: observation
                a: action taken
                r: reward after action
        """
      
        self.episode_observations.append(s)
     
        self.episode_rewards.append(r)
       

        # Store actions as list of arrays
        # e.g. for n_y = 2 -> [ array([ 1.,  0.]), array([ 0.,  1.]), array([ 0.,  1.]), array([ 1.,  0.]) ]
        action = np.zeros(self.n_y)
        if a == 99:
            action[0] = 0
        else:
            action[a] = 1
        self.episode_actions.append(action)
        return self.episode_observations,self.episode_actions, self.episode_rewards
     


    def choose_action(self, observation):
        """
            Choose action based on observation

            Arguments:
                observation: array of state, has shape (num_features)

            Returns: index of action we want to choose
        """
        # Reshape observation to (num_features, 1)
        observation = observation[:, np.newaxis]

        # Run forward propagation to get softmax probabilities
        prob_weights = self.sess.run(self.outputs_softmax, feed_dict = {self.X: observation})
        print("-----------weights: ",prob_weights)
        # Select action using a biased sample
        # this will return the index of the action we've sampled
        action = np.random.choice(range(len(prob_weights.ravel())), p=prob_weights.ravel())
        return action


    def choose_action1(self, observation,matrixOfChanAlloc,nOfstep):
        """
            Choose action based on observation

            Arguments:
                observation: array of state, has shape (num_features)

            Returns: index of action we want to choose
        """
        # Reshape observation to (num_features, 1)
        observation = observation[:, np.newaxis]
        
        ii = np.where(matrixOfChanAlloc[int(observation[0]),:].ravel() == 0)
       
        if ii[0].size ==0:
            return 99

        # Run forward propagation to get softmax probabilities
        prob_weights = self.sess.run(self.outputs_softmax, feed_dict = {self.X: observation})
#        print(prob_weights[0])
        for n in range(len(ii[0])):
            ii[0][n] = ii[0][n] + int(observation[0]*self.nOfChannel)
        if sum(prob_weights[0][ii]) > 0:
            prob_weights = prob_weights[0][ii]/sum(prob_weights[0][ii])
        else:
            prob_weights = prob_weights[0][ii]
        # Select action using a biased sample
        # this will return the index of the action we've sampled
        # try:
        
        if random.random() <= self.ep and sum(prob_weights)>0:
            # action = np.random.choice(range(len(prob_weights.ravel())), p=prob_weights.ravel())
            action = random.randint(0, len(prob_weights.ravel())-1)
        else:
            # action = np.random.choice(range(len(prob_weights.ravel())), p=prob_weights.ravel())
            action = range(len(prob_weights.ravel()))[np.where(prob_weights == max(prob_weights))[0][0]]
        action = ii[0][action] - int(observation[0]*self.nOfChannel)

        matrixOfChanAlloc[int(observation[0][0])][action] = 1


        return action#资源编号




    # def learn(self):
    #     # Discount and normalize episode reward
    #     discounted_episode_rewards_norm = self.discount_and_norm_rewards()

    #     # Train on episode
    #     self.sess.run(self.train_op, feed_dict={
    #          self.X: np.vstack(self.episode_observations).T,
    #          self.Y: np.vstack(np.array(self.episode_actions)).T,
    #          self.discounted_episode_rewards_norm: discounted_episode_rewards_norm,
    #     })

    #     # Reset the episode data
    #     self.episode_observations, self.episode_actions, self.episode_rewards  = [], [], []

    #     # Save checkpoint
    #     if self.save_path is not None:
    #         save_path = self.saver.save(self.sess, self.save_path)
    #         print("Model saved in file: %s" % save_path)

    #     return discounted_episode_rewards_norm

    # def discount_and_norm_rewards(self):
        
    #     discounted_episode_rewards = np.zeros_like(self.episode_rewards)
    #     cumulative = 0
    #     for t in reversed(range(len(self.episode_rewards))):
    #         cumulative = cumulative * self.gamma + self.episode_rewards[t]
    #         discounted_episode_rewards[t] = cumulative
        
    #     # discounted_episode_rewards -= np.mean(discounted_episode_rewards)
    #     # discounted_episode_rewards /= np.std(discounted_episode_rewards)
    #     return discounted_episode_rewards

    def learn(self):
        #由于不能即时获得奖励，需要进行错位
        reward_list = []
        state_list = []
        action_list= []
        list1 = []
        list2 = []
        list3 = []
        for l in range(len(self.episode_rewards)):
            if self.episode_rewards[l] == 0:
                list1.append(0)
                list2.append(self.episode_observations[l])
                list3.append(self.episode_actions[l])
            else:
                list1.append(self.episode_rewards[l])
                list2.append(self.episode_observations[l])
                list3.append(self.episode_actions[l])
                reward_list.append(list1)
                state_list.append(list2)
                action_list.append(list3)
                list1 = []
                list2 = []
                list3 = []
        for i in range(len(reward_list)-3):
            reward_list[i][-1] = reward_list[i+3][-1]
        s = []
        a = []
        r = []
        for i in range(len(reward_list)-3,len(reward_list)-1,1):
            
            print(len(state_list))
            for j in range(len(state_list[i])):
                
                s.append(state_list[i][j])
                a.append(action_list[i][j])
                r.append(reward_list[i][j])


        for i in range(len(reward_list)-3):
            # Discount and normalize episode reward
            discounted_episode_rewards_norm = self.discount_and_norm_rewards(reward_list[i])

            # Train on episode
            self.sess.run(self.train_op, feed_dict={
                self.X: np.vstack(state_list[i]).T,
                self.Y: np.vstack(action_list[i]).T,
                self.discounted_episode_rewards_norm: discounted_episode_rewards_norm,
            })

        # Reset the episode data
        self.episode_observations = s#self.episode_observations[len(self.episode_observations)-3:]
        self.episode_actions = a#self.episode_actions[len(self.episode_actions)-3:]
        self.episode_rewards = r#self.episode_rewards[0:3]

        print(len(self.episode_observations))
        print(len(self.episode_rewards))

        # Save checkpoint
        if self.save_path is not None:
            save_path = self.saver.save(self.sess, self.save_path)
            print("Model saved in file: %s" % save_path)

        # return discounted_episode_rewards_norm

    def discount_and_norm_rewards(self,r):
        discounted_episode_rewards = np.zeros_like(r)
        cumulative = 0
        for t in reversed(range(len(r))):
            cumulative = cumulative * self.gamma + self.episode_rewards[t]
            discounted_episode_rewards[t] = cumulative

        discounted_episode_rewards -= np.mean(discounted_episode_rewards)
        # discounted_episode_rewards /= np.std(discounted_episode_rewards)
        return discounted_episode_rewards


    def build_network(self):
        # Create placeholders
        with tf.name_scope('inputs'):
            self.X = tf.placeholder(tf.float32, shape=(self.n_x, None), name="X")
            self.Y = tf.placeholder(tf.float32, shape=(self.n_y, None), name="Y")
            self.discounted_episode_rewards_norm = tf.placeholder(tf.float32, [None, ], name="actions_value")

        # Initialize parameters
        units_layer_1 = 200
        units_layer_2 = 500
        units_layer_3 = 200
        units_output_layer = self.n_y
        with tf.name_scope('parameters'):
            W1 = tf.get_variable("W1", [units_layer_1, self.n_x], initializer = tf.contrib.layers.xavier_initializer(seed=1))
            b1 = tf.get_variable("b1", [units_layer_1, 1], initializer = tf.contrib.layers.xavier_initializer(seed=1))
            W2 = tf.get_variable("W2", [units_layer_2, units_layer_1], initializer = tf.contrib.layers.xavier_initializer(seed=1))
            b2 = tf.get_variable("b2", [units_layer_2, 1], initializer = tf.contrib.layers.xavier_initializer(seed=1))
            W3 = tf.get_variable("W3", [units_layer_3, units_layer_2], initializer = tf.contrib.layers.xavier_initializer(seed=1))
            b3 = tf.get_variable("b3", [units_layer_3, 1], initializer = tf.contrib.layers.xavier_initializer(seed=1))
            W4 = tf.get_variable("W4", [self.n_y, units_layer_3], initializer = tf.contrib.layers.xavier_initializer(seed=1))
            b4 = tf.get_variable("b4", [self.n_y, 1], initializer = tf.contrib.layers.xavier_initializer(seed=1))

        # Forward prop
        with tf.name_scope('layer_1'):
            Z1 = tf.add(tf.matmul(W1,self.X), b1)
            A1 = tf.nn.relu(Z1)
        with tf.name_scope('layer_2'):
            Z2 = tf.add(tf.matmul(W2, A1), b2)
            A2 = tf.nn.relu(Z2)
        with tf.name_scope('layer_3'):
            Z3 = tf.add(tf.matmul(W3, A2), b3)
            A3 = tf.nn.relu(Z3)
        with tf.name_scope('layer_4'):
            Z4 = tf.add(tf.matmul(W4, A3), b4)
            A4 = tf.nn.softmax(Z4)

        # Softmax outputs, we need to transpose as tensorflow nn functions expects them in this shape
        logits = tf.transpose(Z4)
        labels = tf.transpose(self.Y)
        self.outputs_softmax = tf.nn.softmax(logits, name='A4')

        with tf.name_scope('loss'):
            neg_log_prob = tf.nn.softmax_cross_entropy_with_logits(logits=logits, labels=labels)
            loss = tf.reduce_mean(neg_log_prob * self.discounted_episode_rewards_norm)  # reward guided loss

        with tf.name_scope('train'):
            self.train_op = tf.train.AdamOptimizer(self.lr).minimize(loss)

    def plot_cost(self):
        import matplotlib
        matplotlib.use("MacOSX")
        import matplotlib.pyplot as plt
        plt.plot(np.arange(len(self.cost_history)), self.cost_history)
        plt.ylabel('Cost')
        plt.xlabel('Training Steps')
        plt.show()
