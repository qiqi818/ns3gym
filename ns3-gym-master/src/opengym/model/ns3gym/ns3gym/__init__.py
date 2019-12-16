from gym.envs.registration import register

register(
    id='ns3-v0',
    entry_point='ns3gym.ns3env:Ns3Env',
)
register(
    id='sateDCA_ENV-v0',
    entry_point='ns3gym.sateDCA:sateDCA_ENV',
)
