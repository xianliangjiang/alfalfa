#include "token_costs.hh"

/*
 * This file is based on
 * [1] libvpx:vp8/encoder/rdopt.c
 * [2] libvpx:vp8/encoder/entropy.c
 * [3] libvpx:vp8/encoder/boolhuff.c:23
 */

const uint16_t vp8_prob_cost[ 256 ] =
{
 2047, 2047, 1791, 1641, 1535, 1452, 1385, 1328, 1279, 1235, 1196, 1161, 1129, 1099, 1072, 1046,
 1023, 1000, 979,  959,  940,  922,  905,  889,  873,  858,  843,  829,  816,  803,  790,  778,
 767,  755,  744,  733,  723,  713,  703,  693,  684,  675,  666,  657,  649,  641,  633,  625,
 617,  609,  602,  594,  587,  580,  573,  567,  560,  553,  547,  541,  534,  528,  522,  516,
 511,  505,  499,  494,  488,  483,  477,  472,  467,  462,  457,  452,  447,  442,  437,  433,
 428,  424,  419,  415,  410,  406,  401,  397,  393,  389,  385,  381,  377,  373,  369,  365,
 361,  357,  353,  349,  346,  342,  338,  335,  331,  328,  324,  321,  317,  314,  311,  307,
 304,  301,  297,  294,  291,  288,  285,  281,  278,  275,  272,  269,  266,  263,  260,  257,
 255,  252,  249,  246,  243,  240,  238,  235,  232,  229,  227,  224,  221,  219,  216,  214,
 211,  208,  206,  203,  201,  198,  196,  194,  191,  189,  186,  184,  181,  179,  177,  174,
 172,  170,  168,  165,  163,  161,  159,  156,  154,  152,  150,  148,  145,  143,  141,  139,
 137,  135,  133,  131,  129,  127,  125,  123,  121,  119,  117,  115,  113,  111,  109,  107,
 105,  103,  101,  99,   97,   95,   93,   92,   90,   88,   86,   84,   82,   81,   79,   77,
 75,   73,   72,   70,   68,   66,   65,   63,   61,   60,   58,   56,   55,   53,   51,   50,
 48,   46,   45,   43,   41,   40,   38,   37,   35,   33,   32,   30,   29,   27,   25,   24,
 22,   21,   19,   18,   16,   15,   13,   12,   10,   9,    7,    6,    4,    3,    1,    1
};

uint8_t  inline TokenCosts::complement( uint8_t prob ) { return 255 - prob; }
uint16_t inline TokenCosts::cost_zero( uint8_t prob )  { return vp8_prob_cost[ prob ]; }
uint16_t inline TokenCosts::cost_one( uint8_t prob )   { return vp8_prob_cost[ complement( prob ) ]; }
uint16_t inline TokenCosts::cost_bit( uint8_t prob, bool b ) { return cost_zero( b ? complement( prob ): prob ); }

/* Although we didn't use this tree in other parts of the code, token cost
 * computation code can be written much cleaner with the help of this.
 */
const int8_t vp8_coef_tree[ 22 ] =     /* corresponding _CONTEXT_NODEs */
{
  -DCT_EOB_TOKEN, 2,                       /* 0 = EOB */
  -ZERO_TOKEN, 4,                          /* 1 = ZERO */
  -ONE_TOKEN, 6,                           /* 2 = ONE */
  8, 12,                                   /* 3 = LOW_VAL */
  -TWO_TOKEN, 10,                          /* 4 = TWO */
  -THREE_TOKEN, -FOUR_TOKEN,               /* 5 = THREE */
  14, 16,                                  /* 6 = HIGH_LOW */
  -DCT_VAL_CATEGORY1, -DCT_VAL_CATEGORY2,  /* 7 = CAT_ONE */
  18, 20,                                  /* 8 = CAT_THREEFOUR */
  -DCT_VAL_CATEGORY3, -DCT_VAL_CATEGORY4,  /* 9 = CAT_THREE */
  -DCT_VAL_CATEGORY5, -DCT_VAL_CATEGORY6   /* 10 = CAT_FIVE */
};

void TokenCosts::compute_cost( SafeArray<uint16_t, MAX_ENTROPY_TOKENS> & costs_nodes,
                               const SafeArray<Probability, ENTROPY_NODES> & probabilities,
                               size_t tree_index, uint16_t current_cost )
{
  const Probability prob = probabilities.at( tree_index / 2 );

  for ( size_t i = 0; i < 2; i++ ) { // each tree node has two children
    int tree_entry = vp8_coef_tree[ tree_index + i ];
    int16_t cost = current_cost + cost_bit( prob, i );

    if ( tree_entry <= 0 ) { // if it is a zero or negative value, it's a leaf
      costs_nodes.at( -tree_entry ) = cost;
    }
    else { // recursively compute costs for the children of this inner node
      compute_cost( costs_nodes, probabilities, tree_entry, cost );
    }
  }
}

void TokenCosts::fill_costs( const ProbabilityTables & probability_tables )
{
  for ( size_t i = 0; i < BLOCK_TYPES; i++ ) {
    for ( size_t j = 0; j < COEF_BANDS; j++ ) {
      for ( size_t k = 0; k < PREV_COEF_CONTEXTS; k++ ) {
        auto & costs_array = costs.at( i ).at( j ).at( k );
        auto & probabilities = probability_tables.coeff_probs.at( i ).at( j ).at( k );

        if ( k == 0 && j > ( i == 0 ) ) {
          compute_cost( costs_array, probabilities, 2 );
        }
        else {
          compute_cost( costs_array, probabilities );
        }
      }
    }
  }
}

/*
 * Taken from: libvpx:vp8/encoder/dct_token_values.h
 */
static const uint16_t dct_value_cost[ 2048 * 2 ] =
{
    8285, 8277, 8267, 8259, 8253, 8245, 8226, 8218, 8212, 8204, 8194, 8186,
    8180, 8172, 8150, 8142, 8136, 8128, 8118, 8110, 8104, 8096, 8077, 8069,
    8063, 8055, 8045, 8037, 8031, 8023, 7997, 7989, 7983, 7975, 7965, 7957,
    7951, 7943, 7924, 7916, 7910, 7902, 7892, 7884, 7878, 7870, 7848, 7840,
    7834, 7826, 7816, 7808, 7802, 7794, 7775, 7767, 7761, 7753, 7743, 7735,
    7729, 7721, 7923, 7915, 7909, 7901, 7891, 7883, 7877, 7869, 7850, 7842,
    7836, 7828, 7818, 7810, 7804, 7796, 7774, 7766, 7760, 7752, 7742, 7734,
    7728, 7720, 7701, 7693, 7687, 7679, 7669, 7661, 7655, 7647, 7621, 7613,
    7607, 7599, 7589, 7581, 7575, 7567, 7548, 7540, 7534, 7526, 7516, 7508,
    7502, 7494, 7472, 7464, 7458, 7450, 7440, 7432, 7426, 7418, 7399, 7391,
    7385, 7377, 7367, 7359, 7353, 7345, 7479, 7471, 7465, 7457, 7447, 7439,
    7433, 7425, 7406, 7398, 7392, 7384, 7374, 7366, 7360, 7352, 7330, 7322,
    7316, 7308, 7298, 7290, 7284, 7276, 7257, 7249, 7243, 7235, 7225, 7217,
    7211, 7203, 7177, 7169, 7163, 7155, 7145, 7137, 7131, 7123, 7104, 7096,
    7090, 7082, 7072, 7064, 7058, 7050, 7028, 7020, 7014, 7006, 6996, 6988,
    6982, 6974, 6955, 6947, 6941, 6933, 6923, 6915, 6909, 6901, 7632, 7624,
    7618, 7610, 7600, 7592, 7586, 7578, 7559, 7551, 7545, 7537, 7527, 7519,
    7513, 7505, 7483, 7475, 7469, 7461, 7451, 7443, 7437, 7429, 7410, 7402,
    7396, 7388, 7378, 7370, 7364, 7356, 7330, 7322, 7316, 7308, 7298, 7290,
    7284, 7276, 7257, 7249, 7243, 7235, 7225, 7217, 7211, 7203, 7181, 7173,
    7167, 7159, 7149, 7141, 7135, 7127, 7108, 7100, 7094, 7086, 7076, 7068,
    7062, 7054, 7188, 7180, 7174, 7166, 7156, 7148, 7142, 7134, 7115, 7107,
    7101, 7093, 7083, 7075, 7069, 7061, 7039, 7031, 7025, 7017, 7007, 6999,
    6993, 6985, 6966, 6958, 6952, 6944, 6934, 6926, 6920, 6912, 6886, 6878,
    6872, 6864, 6854, 6846, 6840, 6832, 6813, 6805, 6799, 6791, 6781, 6773,
    6767, 6759, 6737, 6729, 6723, 6715, 6705, 6697, 6691, 6683, 6664, 6656,
    6650, 6642, 6632, 6624, 6618, 6610, 6812, 6804, 6798, 6790, 6780, 6772,
    6766, 6758, 6739, 6731, 6725, 6717, 6707, 6699, 6693, 6685, 6663, 6655,
    6649, 6641, 6631, 6623, 6617, 6609, 6590, 6582, 6576, 6568, 6558, 6550,
    6544, 6536, 6510, 6502, 6496, 6488, 6478, 6470, 6464, 6456, 6437, 6429,
    6423, 6415, 6405, 6397, 6391, 6383, 6361, 6353, 6347, 6339, 6329, 6321,
    6315, 6307, 6288, 6280, 6274, 6266, 6256, 6248, 6242, 6234, 6368, 6360,
    6354, 6346, 6336, 6328, 6322, 6314, 6295, 6287, 6281, 6273, 6263, 6255,
    6249, 6241, 6219, 6211, 6205, 6197, 6187, 6179, 6173, 6165, 6146, 6138,
    6132, 6124, 6114, 6106, 6100, 6092, 6066, 6058, 6052, 6044, 6034, 6026,
    6020, 6012, 5993, 5985, 5979, 5971, 5961, 5953, 5947, 5939, 5917, 5909,
    5903, 5895, 5885, 5877, 5871, 5863, 5844, 5836, 5830, 5822, 5812, 5804,
    5798, 5790, 6697, 6689, 6683, 6675, 6665, 6657, 6651, 6643, 6624, 6616,
    6610, 6602, 6592, 6584, 6578, 6570, 6548, 6540, 6534, 6526, 6516, 6508,
    6502, 6494, 6475, 6467, 6461, 6453, 6443, 6435, 6429, 6421, 6395, 6387,
    6381, 6373, 6363, 6355, 6349, 6341, 6322, 6314, 6308, 6300, 6290, 6282,
    6276, 6268, 6246, 6238, 6232, 6224, 6214, 6206, 6200, 6192, 6173, 6165,
    6159, 6151, 6141, 6133, 6127, 6119, 6253, 6245, 6239, 6231, 6221, 6213,
    6207, 6199, 6180, 6172, 6166, 6158, 6148, 6140, 6134, 6126, 6104, 6096,
    6090, 6082, 6072, 6064, 6058, 6050, 6031, 6023, 6017, 6009, 5999, 5991,
    5985, 5977, 5951, 5943, 5937, 5929, 5919, 5911, 5905, 5897, 5878, 5870,
    5864, 5856, 5846, 5838, 5832, 5824, 5802, 5794, 5788, 5780, 5770, 5762,
    5756, 5748, 5729, 5721, 5715, 5707, 5697, 5689, 5683, 5675, 5877, 5869,
    5863, 5855, 5845, 5837, 5831, 5823, 5804, 5796, 5790, 5782, 5772, 5764,
    5758, 5750, 5728, 5720, 5714, 5706, 5696, 5688, 5682, 5674, 5655, 5647,
    5641, 5633, 5623, 5615, 5609, 5601, 5575, 5567, 5561, 5553, 5543, 5535,
    5529, 5521, 5502, 5494, 5488, 5480, 5470, 5462, 5456, 5448, 5426, 5418,
    5412, 5404, 5394, 5386, 5380, 5372, 5353, 5345, 5339, 5331, 5321, 5313,
    5307, 5299, 5433, 5425, 5419, 5411, 5401, 5393, 5387, 5379, 5360, 5352,
    5346, 5338, 5328, 5320, 5314, 5306, 5284, 5276, 5270, 5262, 5252, 5244,
    5238, 5230, 5211, 5203, 5197, 5189, 5179, 5171, 5165, 5157, 5131, 5123,
    5117, 5109, 5099, 5091, 5085, 5077, 5058, 5050, 5044, 5036, 5026, 5018,
    5012, 5004, 4982, 4974, 4968, 4960, 4950, 4942, 4936, 4928, 4909, 4901,
    4895, 4887, 4877, 4869, 4863, 4855, 5586, 5578, 5572, 5564, 5554, 5546,
    5540, 5532, 5513, 5505, 5499, 5491, 5481, 5473, 5467, 5459, 5437, 5429,
    5423, 5415, 5405, 5397, 5391, 5383, 5364, 5356, 5350, 5342, 5332, 5324,
    5318, 5310, 5284, 5276, 5270, 5262, 5252, 5244, 5238, 5230, 5211, 5203,
    5197, 5189, 5179, 5171, 5165, 5157, 5135, 5127, 5121, 5113, 5103, 5095,
    5089, 5081, 5062, 5054, 5048, 5040, 5030, 5022, 5016, 5008, 5142, 5134,
    5128, 5120, 5110, 5102, 5096, 5088, 5069, 5061, 5055, 5047, 5037, 5029,
    5023, 5015, 4993, 4985, 4979, 4971, 4961, 4953, 4947, 4939, 4920, 4912,
    4906, 4898, 4888, 4880, 4874, 4866, 4840, 4832, 4826, 4818, 4808, 4800,
    4794, 4786, 4767, 4759, 4753, 4745, 4735, 4727, 4721, 4713, 4691, 4683,
    4677, 4669, 4659, 4651, 4645, 4637, 4618, 4610, 4604, 4596, 4586, 4578,
    4572, 4564, 4766, 4758, 4752, 4744, 4734, 4726, 4720, 4712, 4693, 4685,
    4679, 4671, 4661, 4653, 4647, 4639, 4617, 4609, 4603, 4595, 4585, 4577,
    4571, 4563, 4544, 4536, 4530, 4522, 4512, 4504, 4498, 4490, 4464, 4456,
    4450, 4442, 4432, 4424, 4418, 4410, 4391, 4383, 4377, 4369, 4359, 4351,
    4345, 4337, 4315, 4307, 4301, 4293, 4283, 4275, 4269, 4261, 4242, 4234,
    4228, 4220, 4210, 4202, 4196, 4188, 4322, 4314, 4308, 4300, 4290, 4282,
    4276, 4268, 4249, 4241, 4235, 4227, 4217, 4209, 4203, 4195, 4173, 4165,
    4159, 4151, 4141, 4133, 4127, 4119, 4100, 4092, 4086, 4078, 4068, 4060,
    4054, 4046, 4020, 4012, 4006, 3998, 3988, 3980, 3974, 3966, 3947, 3939,
    3933, 3925, 3915, 3907, 3901, 3893, 3871, 3863, 3857, 3849, 3839, 3831,
    3825, 3817, 3798, 3790, 3784, 3776, 3766, 3758, 3752, 3744, 6697, 6689,
    6683, 6675, 6665, 6657, 6651, 6643, 6624, 6616, 6610, 6602, 6592, 6584,
    6578, 6570, 6548, 6540, 6534, 6526, 6516, 6508, 6502, 6494, 6475, 6467,
    6461, 6453, 6443, 6435, 6429, 6421, 6395, 6387, 6381, 6373, 6363, 6355,
    6349, 6341, 6322, 6314, 6308, 6300, 6290, 6282, 6276, 6268, 6246, 6238,
    6232, 6224, 6214, 6206, 6200, 6192, 6173, 6165, 6159, 6151, 6141, 6133,
    6127, 6119, 6253, 6245, 6239, 6231, 6221, 6213, 6207, 6199, 6180, 6172,
    6166, 6158, 6148, 6140, 6134, 6126, 6104, 6096, 6090, 6082, 6072, 6064,
    6058, 6050, 6031, 6023, 6017, 6009, 5999, 5991, 5985, 5977, 5951, 5943,
    5937, 5929, 5919, 5911, 5905, 5897, 5878, 5870, 5864, 5856, 5846, 5838,
    5832, 5824, 5802, 5794, 5788, 5780, 5770, 5762, 5756, 5748, 5729, 5721,
    5715, 5707, 5697, 5689, 5683, 5675, 5877, 5869, 5863, 5855, 5845, 5837,
    5831, 5823, 5804, 5796, 5790, 5782, 5772, 5764, 5758, 5750, 5728, 5720,
    5714, 5706, 5696, 5688, 5682, 5674, 5655, 5647, 5641, 5633, 5623, 5615,
    5609, 5601, 5575, 5567, 5561, 5553, 5543, 5535, 5529, 5521, 5502, 5494,
    5488, 5480, 5470, 5462, 5456, 5448, 5426, 5418, 5412, 5404, 5394, 5386,
    5380, 5372, 5353, 5345, 5339, 5331, 5321, 5313, 5307, 5299, 5433, 5425,
    5419, 5411, 5401, 5393, 5387, 5379, 5360, 5352, 5346, 5338, 5328, 5320,
    5314, 5306, 5284, 5276, 5270, 5262, 5252, 5244, 5238, 5230, 5211, 5203,
    5197, 5189, 5179, 5171, 5165, 5157, 5131, 5123, 5117, 5109, 5099, 5091,
    5085, 5077, 5058, 5050, 5044, 5036, 5026, 5018, 5012, 5004, 4982, 4974,
    4968, 4960, 4950, 4942, 4936, 4928, 4909, 4901, 4895, 4887, 4877, 4869,
    4863, 4855, 5586, 5578, 5572, 5564, 5554, 5546, 5540, 5532, 5513, 5505,
    5499, 5491, 5481, 5473, 5467, 5459, 5437, 5429, 5423, 5415, 5405, 5397,
    5391, 5383, 5364, 5356, 5350, 5342, 5332, 5324, 5318, 5310, 5284, 5276,
    5270, 5262, 5252, 5244, 5238, 5230, 5211, 5203, 5197, 5189, 5179, 5171,
    5165, 5157, 5135, 5127, 5121, 5113, 5103, 5095, 5089, 5081, 5062, 5054,
    5048, 5040, 5030, 5022, 5016, 5008, 5142, 5134, 5128, 5120, 5110, 5102,
    5096, 5088, 5069, 5061, 5055, 5047, 5037, 5029, 5023, 5015, 4993, 4985,
    4979, 4971, 4961, 4953, 4947, 4939, 4920, 4912, 4906, 4898, 4888, 4880,
    4874, 4866, 4840, 4832, 4826, 4818, 4808, 4800, 4794, 4786, 4767, 4759,
    4753, 4745, 4735, 4727, 4721, 4713, 4691, 4683, 4677, 4669, 4659, 4651,
    4645, 4637, 4618, 4610, 4604, 4596, 4586, 4578, 4572, 4564, 4766, 4758,
    4752, 4744, 4734, 4726, 4720, 4712, 4693, 4685, 4679, 4671, 4661, 4653,
    4647, 4639, 4617, 4609, 4603, 4595, 4585, 4577, 4571, 4563, 4544, 4536,
    4530, 4522, 4512, 4504, 4498, 4490, 4464, 4456, 4450, 4442, 4432, 4424,
    4418, 4410, 4391, 4383, 4377, 4369, 4359, 4351, 4345, 4337, 4315, 4307,
    4301, 4293, 4283, 4275, 4269, 4261, 4242, 4234, 4228, 4220, 4210, 4202,
    4196, 4188, 4322, 4314, 4308, 4300, 4290, 4282, 4276, 4268, 4249, 4241,
    4235, 4227, 4217, 4209, 4203, 4195, 4173, 4165, 4159, 4151, 4141, 4133,
    4127, 4119, 4100, 4092, 4086, 4078, 4068, 4060, 4054, 4046, 4020, 4012,
    4006, 3998, 3988, 3980, 3974, 3966, 3947, 3939, 3933, 3925, 3915, 3907,
    3901, 3893, 3871, 3863, 3857, 3849, 3839, 3831, 3825, 3817, 3798, 3790,
    3784, 3776, 3766, 3758, 3752, 3744, 4651, 4643, 4637, 4629, 4619, 4611,
    4605, 4597, 4578, 4570, 4564, 4556, 4546, 4538, 4532, 4524, 4502, 4494,
    4488, 4480, 4470, 4462, 4456, 4448, 4429, 4421, 4415, 4407, 4397, 4389,
    4383, 4375, 4349, 4341, 4335, 4327, 4317, 4309, 4303, 4295, 4276, 4268,
    4262, 4254, 4244, 4236, 4230, 4222, 4200, 4192, 4186, 4178, 4168, 4160,
    4154, 4146, 4127, 4119, 4113, 4105, 4095, 4087, 4081, 4073, 4207, 4199,
    4193, 4185, 4175, 4167, 4161, 4153, 4134, 4126, 4120, 4112, 4102, 4094,
    4088, 4080, 4058, 4050, 4044, 4036, 4026, 4018, 4012, 4004, 3985, 3977,
    3971, 3963, 3953, 3945, 3939, 3931, 3905, 3897, 3891, 3883, 3873, 3865,
    3859, 3851, 3832, 3824, 3818, 3810, 3800, 3792, 3786, 3778, 3756, 3748,
    3742, 3734, 3724, 3716, 3710, 3702, 3683, 3675, 3669, 3661, 3651, 3643,
    3637, 3629, 3831, 3823, 3817, 3809, 3799, 3791, 3785, 3777, 3758, 3750,
    3744, 3736, 3726, 3718, 3712, 3704, 3682, 3674, 3668, 3660, 3650, 3642,
    3636, 3628, 3609, 3601, 3595, 3587, 3577, 3569, 3563, 3555, 3529, 3521,
    3515, 3507, 3497, 3489, 3483, 3475, 3456, 3448, 3442, 3434, 3424, 3416,
    3410, 3402, 3380, 3372, 3366, 3358, 3348, 3340, 3334, 3326, 3307, 3299,
    3293, 3285, 3275, 3267, 3261, 3253, 3387, 3379, 3373, 3365, 3355, 3347,
    3341, 3333, 3314, 3306, 3300, 3292, 3282, 3274, 3268, 3260, 3238, 3230,
    3224, 3216, 3206, 3198, 3192, 3184, 3165, 3157, 3151, 3143, 3133, 3125,
    3119, 3111, 3085, 3077, 3071, 3063, 3053, 3045, 3039, 3031, 3012, 3004,
    2998, 2990, 2980, 2972, 2966, 2958, 2936, 2928, 2922, 2914, 2904, 2896,
    2890, 2882, 2863, 2855, 2849, 2841, 2831, 2823, 2817, 2809, 3540, 3532,
    3526, 3518, 3508, 3500, 3494, 3486, 3467, 3459, 3453, 3445, 3435, 3427,
    3421, 3413, 3391, 3383, 3377, 3369, 3359, 3351, 3345, 3337, 3318, 3310,
    3304, 3296, 3286, 3278, 3272, 3264, 3238, 3230, 3224, 3216, 3206, 3198,
    3192, 3184, 3165, 3157, 3151, 3143, 3133, 3125, 3119, 3111, 3089, 3081,
    3075, 3067, 3057, 3049, 3043, 3035, 3016, 3008, 3002, 2994, 2984, 2976,
    2970, 2962, 3096, 3088, 3082, 3074, 3064, 3056, 3050, 3042, 3023, 3015,
    3009, 3001, 2991, 2983, 2977, 2969, 2947, 2939, 2933, 2925, 2915, 2907,
    2901, 2893, 2874, 2866, 2860, 2852, 2842, 2834, 2828, 2820, 2794, 2786,
    2780, 2772, 2762, 2754, 2748, 2740, 2721, 2713, 2707, 2699, 2689, 2681,
    2675, 2667, 2645, 2637, 2631, 2623, 2613, 2605, 2599, 2591, 2572, 2564,
    2558, 2550, 2540, 2532, 2526, 2518, 2720, 2712, 2706, 2698, 2688, 2680,
    2674, 2666, 2647, 2639, 2633, 2625, 2615, 2607, 2601, 2593, 2571, 2563,
    2557, 2549, 2539, 2531, 2525, 2517, 2498, 2490, 2484, 2476, 2466, 2458,
    2452, 2444, 2418, 2410, 2404, 2396, 2386, 2378, 2372, 2364, 2345, 2337,
    2331, 2323, 2313, 2305, 2299, 2291, 2269, 2261, 2255, 2247, 2237, 2229,
    2223, 2215, 2196, 2188, 2182, 2174, 2164, 2156, 2150, 2142, 2276, 2268,
    2262, 2254, 2244, 2236, 2230, 2222, 2203, 2195, 2189, 2181, 2171, 2163,
    2157, 2149, 2127, 2119, 2113, 2105, 2095, 2087, 2081, 2073, 2054, 2046,
    2040, 2032, 2022, 2014, 2008, 2000, 1974, 1966, 1960, 1952, 1942, 1934,
    1928, 1920, 1901, 1893, 1887, 1879, 1869, 1861, 1855, 1847, 1825, 1817,
    1811, 1803, 1793, 1785, 1779, 1771, 1752, 1744, 1738, 1730, 1720, 1712,
    1706, 1698, 1897, 1883, 1860, 1846, 1819, 1805, 1782, 1768, 1723, 1709,
    1686, 1672, 1645, 1631, 1608, 1594, 1574, 1560, 1537, 1523, 1496, 1482,
    1459, 1445, 1400, 1386, 1363, 1349, 1322, 1308, 1285, 1271, 1608, 1565,
    1535, 1492, 1446, 1403, 1373, 1330, 1312, 1269, 1239, 1196, 1150, 1107,
    1077, 1034, 1291, 1218, 1171, 1098, 1015, 942, 895, 822, 953, 850,
    729, 626, 618, 431, 257, 257, 257, 257, 0, 255, 255, 255,
    255, 429, 616, 624, 727, 848, 951, 820, 893, 940, 1013, 1096,
    1169, 1216, 1289, 1032, 1075, 1105, 1148, 1194, 1237, 1267, 1310, 1328,
    1371, 1401, 1444, 1490, 1533, 1563, 1606, 1269, 1283, 1306, 1320, 1347,
    1361, 1384, 1398, 1443, 1457, 1480, 1494, 1521, 1535, 1558, 1572, 1592,
    1606, 1629, 1643, 1670, 1684, 1707, 1721, 1766, 1780, 1803, 1817, 1844,
    1858, 1881, 1895, 1696, 1704, 1710, 1718, 1728, 1736, 1742, 1750, 1769,
    1777, 1783, 1791, 1801, 1809, 1815, 1823, 1845, 1853, 1859, 1867, 1877,
    1885, 1891, 1899, 1918, 1926, 1932, 1940, 1950, 1958, 1964, 1972, 1998,
    2006, 2012, 2020, 2030, 2038, 2044, 2052, 2071, 2079, 2085, 2093, 2103,
    2111, 2117, 2125, 2147, 2155, 2161, 2169, 2179, 2187, 2193, 2201, 2220,
    2228, 2234, 2242, 2252, 2260, 2266, 2274, 2140, 2148, 2154, 2162, 2172,
    2180, 2186, 2194, 2213, 2221, 2227, 2235, 2245, 2253, 2259, 2267, 2289,
    2297, 2303, 2311, 2321, 2329, 2335, 2343, 2362, 2370, 2376, 2384, 2394,
    2402, 2408, 2416, 2442, 2450, 2456, 2464, 2474, 2482, 2488, 2496, 2515,
    2523, 2529, 2537, 2547, 2555, 2561, 2569, 2591, 2599, 2605, 2613, 2623,
    2631, 2637, 2645, 2664, 2672, 2678, 2686, 2696, 2704, 2710, 2718, 2516,
    2524, 2530, 2538, 2548, 2556, 2562, 2570, 2589, 2597, 2603, 2611, 2621,
    2629, 2635, 2643, 2665, 2673, 2679, 2687, 2697, 2705, 2711, 2719, 2738,
    2746, 2752, 2760, 2770, 2778, 2784, 2792, 2818, 2826, 2832, 2840, 2850,
    2858, 2864, 2872, 2891, 2899, 2905, 2913, 2923, 2931, 2937, 2945, 2967,
    2975, 2981, 2989, 2999, 3007, 3013, 3021, 3040, 3048, 3054, 3062, 3072,
    3080, 3086, 3094, 2960, 2968, 2974, 2982, 2992, 3000, 3006, 3014, 3033,
    3041, 3047, 3055, 3065, 3073, 3079, 3087, 3109, 3117, 3123, 3131, 3141,
    3149, 3155, 3163, 3182, 3190, 3196, 3204, 3214, 3222, 3228, 3236, 3262,
    3270, 3276, 3284, 3294, 3302, 3308, 3316, 3335, 3343, 3349, 3357, 3367,
    3375, 3381, 3389, 3411, 3419, 3425, 3433, 3443, 3451, 3457, 3465, 3484,
    3492, 3498, 3506, 3516, 3524, 3530, 3538, 2807, 2815, 2821, 2829, 2839,
    2847, 2853, 2861, 2880, 2888, 2894, 2902, 2912, 2920, 2926, 2934, 2956,
    2964, 2970, 2978, 2988, 2996, 3002, 3010, 3029, 3037, 3043, 3051, 3061,
    3069, 3075, 3083, 3109, 3117, 3123, 3131, 3141, 3149, 3155, 3163, 3182,
    3190, 3196, 3204, 3214, 3222, 3228, 3236, 3258, 3266, 3272, 3280, 3290,
    3298, 3304, 3312, 3331, 3339, 3345, 3353, 3363, 3371, 3377, 3385, 3251,
    3259, 3265, 3273, 3283, 3291, 3297, 3305, 3324, 3332, 3338, 3346, 3356,
    3364, 3370, 3378, 3400, 3408, 3414, 3422, 3432, 3440, 3446, 3454, 3473,
    3481, 3487, 3495, 3505, 3513, 3519, 3527, 3553, 3561, 3567, 3575, 3585,
    3593, 3599, 3607, 3626, 3634, 3640, 3648, 3658, 3666, 3672, 3680, 3702,
    3710, 3716, 3724, 3734, 3742, 3748, 3756, 3775, 3783, 3789, 3797, 3807,
    3815, 3821, 3829, 3627, 3635, 3641, 3649, 3659, 3667, 3673, 3681, 3700,
    3708, 3714, 3722, 3732, 3740, 3746, 3754, 3776, 3784, 3790, 3798, 3808,
    3816, 3822, 3830, 3849, 3857, 3863, 3871, 3881, 3889, 3895, 3903, 3929,
    3937, 3943, 3951, 3961, 3969, 3975, 3983, 4002, 4010, 4016, 4024, 4034,
    4042, 4048, 4056, 4078, 4086, 4092, 4100, 4110, 4118, 4124, 4132, 4151,
    4159, 4165, 4173, 4183, 4191, 4197, 4205, 4071, 4079, 4085, 4093, 4103,
    4111, 4117, 4125, 4144, 4152, 4158, 4166, 4176, 4184, 4190, 4198, 4220,
    4228, 4234, 4242, 4252, 4260, 4266, 4274, 4293, 4301, 4307, 4315, 4325,
    4333, 4339, 4347, 4373, 4381, 4387, 4395, 4405, 4413, 4419, 4427, 4446,
    4454, 4460, 4468, 4478, 4486, 4492, 4500, 4522, 4530, 4536, 4544, 4554,
    4562, 4568, 4576, 4595, 4603, 4609, 4617, 4627, 4635, 4641, 4649, 3742,
    3750, 3756, 3764, 3774, 3782, 3788, 3796, 3815, 3823, 3829, 3837, 3847,
    3855, 3861, 3869, 3891, 3899, 3905, 3913, 3923, 3931, 3937, 3945, 3964,
    3972, 3978, 3986, 3996, 4004, 4010, 4018, 4044, 4052, 4058, 4066, 4076,
    4084, 4090, 4098, 4117, 4125, 4131, 4139, 4149, 4157, 4163, 4171, 4193,
    4201, 4207, 4215, 4225, 4233, 4239, 4247, 4266, 4274, 4280, 4288, 4298,
    4306, 4312, 4320, 4186, 4194, 4200, 4208, 4218, 4226, 4232, 4240, 4259,
    4267, 4273, 4281, 4291, 4299, 4305, 4313, 4335, 4343, 4349, 4357, 4367,
    4375, 4381, 4389, 4408, 4416, 4422, 4430, 4440, 4448, 4454, 4462, 4488,
    4496, 4502, 4510, 4520, 4528, 4534, 4542, 4561, 4569, 4575, 4583, 4593,
    4601, 4607, 4615, 4637, 4645, 4651, 4659, 4669, 4677, 4683, 4691, 4710,
    4718, 4724, 4732, 4742, 4750, 4756, 4764, 4562, 4570, 4576, 4584, 4594,
    4602, 4608, 4616, 4635, 4643, 4649, 4657, 4667, 4675, 4681, 4689, 4711,
    4719, 4725, 4733, 4743, 4751, 4757, 4765, 4784, 4792, 4798, 4806, 4816,
    4824, 4830, 4838, 4864, 4872, 4878, 4886, 4896, 4904, 4910, 4918, 4937,
    4945, 4951, 4959, 4969, 4977, 4983, 4991, 5013, 5021, 5027, 5035, 5045,
    5053, 5059, 5067, 5086, 5094, 5100, 5108, 5118, 5126, 5132, 5140, 5006,
    5014, 5020, 5028, 5038, 5046, 5052, 5060, 5079, 5087, 5093, 5101, 5111,
    5119, 5125, 5133, 5155, 5163, 5169, 5177, 5187, 5195, 5201, 5209, 5228,
    5236, 5242, 5250, 5260, 5268, 5274, 5282, 5308, 5316, 5322, 5330, 5340,
    5348, 5354, 5362, 5381, 5389, 5395, 5403, 5413, 5421, 5427, 5435, 5457,
    5465, 5471, 5479, 5489, 5497, 5503, 5511, 5530, 5538, 5544, 5552, 5562,
    5570, 5576, 5584, 4853, 4861, 4867, 4875, 4885, 4893, 4899, 4907, 4926,
    4934, 4940, 4948, 4958, 4966, 4972, 4980, 5002, 5010, 5016, 5024, 5034,
    5042, 5048, 5056, 5075, 5083, 5089, 5097, 5107, 5115, 5121, 5129, 5155,
    5163, 5169, 5177, 5187, 5195, 5201, 5209, 5228, 5236, 5242, 5250, 5260,
    5268, 5274, 5282, 5304, 5312, 5318, 5326, 5336, 5344, 5350, 5358, 5377,
    5385, 5391, 5399, 5409, 5417, 5423, 5431, 5297, 5305, 5311, 5319, 5329,
    5337, 5343, 5351, 5370, 5378, 5384, 5392, 5402, 5410, 5416, 5424, 5446,
    5454, 5460, 5468, 5478, 5486, 5492, 5500, 5519, 5527, 5533, 5541, 5551,
    5559, 5565, 5573, 5599, 5607, 5613, 5621, 5631, 5639, 5645, 5653, 5672,
    5680, 5686, 5694, 5704, 5712, 5718, 5726, 5748, 5756, 5762, 5770, 5780,
    5788, 5794, 5802, 5821, 5829, 5835, 5843, 5853, 5861, 5867, 5875, 5673,
    5681, 5687, 5695, 5705, 5713, 5719, 5727, 5746, 5754, 5760, 5768, 5778,
    5786, 5792, 5800, 5822, 5830, 5836, 5844, 5854, 5862, 5868, 5876, 5895,
    5903, 5909, 5917, 5927, 5935, 5941, 5949, 5975, 5983, 5989, 5997, 6007,
    6015, 6021, 6029, 6048, 6056, 6062, 6070, 6080, 6088, 6094, 6102, 6124,
    6132, 6138, 6146, 6156, 6164, 6170, 6178, 6197, 6205, 6211, 6219, 6229,
    6237, 6243, 6251, 6117, 6125, 6131, 6139, 6149, 6157, 6163, 6171, 6190,
    6198, 6204, 6212, 6222, 6230, 6236, 6244, 6266, 6274, 6280, 6288, 6298,
    6306, 6312, 6320, 6339, 6347, 6353, 6361, 6371, 6379, 6385, 6393, 6419,
    6427, 6433, 6441, 6451, 6459, 6465, 6473, 6492, 6500, 6506, 6514, 6524,
    6532, 6538, 6546, 6568, 6576, 6582, 6590, 6600, 6608, 6614, 6622, 6641,
    6649, 6655, 6663, 6673, 6681, 6687, 6695, 3742, 3750, 3756, 3764, 3774,
    3782, 3788, 3796, 3815, 3823, 3829, 3837, 3847, 3855, 3861, 3869, 3891,
    3899, 3905, 3913, 3923, 3931, 3937, 3945, 3964, 3972, 3978, 3986, 3996,
    4004, 4010, 4018, 4044, 4052, 4058, 4066, 4076, 4084, 4090, 4098, 4117,
    4125, 4131, 4139, 4149, 4157, 4163, 4171, 4193, 4201, 4207, 4215, 4225,
    4233, 4239, 4247, 4266, 4274, 4280, 4288, 4298, 4306, 4312, 4320, 4186,
    4194, 4200, 4208, 4218, 4226, 4232, 4240, 4259, 4267, 4273, 4281, 4291,
    4299, 4305, 4313, 4335, 4343, 4349, 4357, 4367, 4375, 4381, 4389, 4408,
    4416, 4422, 4430, 4440, 4448, 4454, 4462, 4488, 4496, 4502, 4510, 4520,
    4528, 4534, 4542, 4561, 4569, 4575, 4583, 4593, 4601, 4607, 4615, 4637,
    4645, 4651, 4659, 4669, 4677, 4683, 4691, 4710, 4718, 4724, 4732, 4742,
    4750, 4756, 4764, 4562, 4570, 4576, 4584, 4594, 4602, 4608, 4616, 4635,
    4643, 4649, 4657, 4667, 4675, 4681, 4689, 4711, 4719, 4725, 4733, 4743,
    4751, 4757, 4765, 4784, 4792, 4798, 4806, 4816, 4824, 4830, 4838, 4864,
    4872, 4878, 4886, 4896, 4904, 4910, 4918, 4937, 4945, 4951, 4959, 4969,
    4977, 4983, 4991, 5013, 5021, 5027, 5035, 5045, 5053, 5059, 5067, 5086,
    5094, 5100, 5108, 5118, 5126, 5132, 5140, 5006, 5014, 5020, 5028, 5038,
    5046, 5052, 5060, 5079, 5087, 5093, 5101, 5111, 5119, 5125, 5133, 5155,
    5163, 5169, 5177, 5187, 5195, 5201, 5209, 5228, 5236, 5242, 5250, 5260,
    5268, 5274, 5282, 5308, 5316, 5322, 5330, 5340, 5348, 5354, 5362, 5381,
    5389, 5395, 5403, 5413, 5421, 5427, 5435, 5457, 5465, 5471, 5479, 5489,
    5497, 5503, 5511, 5530, 5538, 5544, 5552, 5562, 5570, 5576, 5584, 4853,
    4861, 4867, 4875, 4885, 4893, 4899, 4907, 4926, 4934, 4940, 4948, 4958,
    4966, 4972, 4980, 5002, 5010, 5016, 5024, 5034, 5042, 5048, 5056, 5075,
    5083, 5089, 5097, 5107, 5115, 5121, 5129, 5155, 5163, 5169, 5177, 5187,
    5195, 5201, 5209, 5228, 5236, 5242, 5250, 5260, 5268, 5274, 5282, 5304,
    5312, 5318, 5326, 5336, 5344, 5350, 5358, 5377, 5385, 5391, 5399, 5409,
    5417, 5423, 5431, 5297, 5305, 5311, 5319, 5329, 5337, 5343, 5351, 5370,
    5378, 5384, 5392, 5402, 5410, 5416, 5424, 5446, 5454, 5460, 5468, 5478,
    5486, 5492, 5500, 5519, 5527, 5533, 5541, 5551, 5559, 5565, 5573, 5599,
    5607, 5613, 5621, 5631, 5639, 5645, 5653, 5672, 5680, 5686, 5694, 5704,
    5712, 5718, 5726, 5748, 5756, 5762, 5770, 5780, 5788, 5794, 5802, 5821,
    5829, 5835, 5843, 5853, 5861, 5867, 5875, 5673, 5681, 5687, 5695, 5705,
    5713, 5719, 5727, 5746, 5754, 5760, 5768, 5778, 5786, 5792, 5800, 5822,
    5830, 5836, 5844, 5854, 5862, 5868, 5876, 5895, 5903, 5909, 5917, 5927,
    5935, 5941, 5949, 5975, 5983, 5989, 5997, 6007, 6015, 6021, 6029, 6048,
    6056, 6062, 6070, 6080, 6088, 6094, 6102, 6124, 6132, 6138, 6146, 6156,
    6164, 6170, 6178, 6197, 6205, 6211, 6219, 6229, 6237, 6243, 6251, 6117,
    6125, 6131, 6139, 6149, 6157, 6163, 6171, 6190, 6198, 6204, 6212, 6222,
    6230, 6236, 6244, 6266, 6274, 6280, 6288, 6298, 6306, 6312, 6320, 6339,
    6347, 6353, 6361, 6371, 6379, 6385, 6393, 6419, 6427, 6433, 6441, 6451,
    6459, 6465, 6473, 6492, 6500, 6506, 6514, 6524, 6532, 6538, 6546, 6568,
    6576, 6582, 6590, 6600, 6608, 6614, 6622, 6641, 6649, 6655, 6663, 6673,
    6681, 6687, 6695, 5788, 5796, 5802, 5810, 5820, 5828, 5834, 5842, 5861,
    5869, 5875, 5883, 5893, 5901, 5907, 5915, 5937, 5945, 5951, 5959, 5969,
    5977, 5983, 5991, 6010, 6018, 6024, 6032, 6042, 6050, 6056, 6064, 6090,
    6098, 6104, 6112, 6122, 6130, 6136, 6144, 6163, 6171, 6177, 6185, 6195,
    6203, 6209, 6217, 6239, 6247, 6253, 6261, 6271, 6279, 6285, 6293, 6312,
    6320, 6326, 6334, 6344, 6352, 6358, 6366, 6232, 6240, 6246, 6254, 6264,
    6272, 6278, 6286, 6305, 6313, 6319, 6327, 6337, 6345, 6351, 6359, 6381,
    6389, 6395, 6403, 6413, 6421, 6427, 6435, 6454, 6462, 6468, 6476, 6486,
    6494, 6500, 6508, 6534, 6542, 6548, 6556, 6566, 6574, 6580, 6588, 6607,
    6615, 6621, 6629, 6639, 6647, 6653, 6661, 6683, 6691, 6697, 6705, 6715,
    6723, 6729, 6737, 6756, 6764, 6770, 6778, 6788, 6796, 6802, 6810, 6608,
    6616, 6622, 6630, 6640, 6648, 6654, 6662, 6681, 6689, 6695, 6703, 6713,
    6721, 6727, 6735, 6757, 6765, 6771, 6779, 6789, 6797, 6803, 6811, 6830,
    6838, 6844, 6852, 6862, 6870, 6876, 6884, 6910, 6918, 6924, 6932, 6942,
    6950, 6956, 6964, 6983, 6991, 6997, 7005, 7015, 7023, 7029, 7037, 7059,
    7067, 7073, 7081, 7091, 7099, 7105, 7113, 7132, 7140, 7146, 7154, 7164,
    7172, 7178, 7186, 7052, 7060, 7066, 7074, 7084, 7092, 7098, 7106, 7125,
    7133, 7139, 7147, 7157, 7165, 7171, 7179, 7201, 7209, 7215, 7223, 7233,
    7241, 7247, 7255, 7274, 7282, 7288, 7296, 7306, 7314, 7320, 7328, 7354,
    7362, 7368, 7376, 7386, 7394, 7400, 7408, 7427, 7435, 7441, 7449, 7459,
    7467, 7473, 7481, 7503, 7511, 7517, 7525, 7535, 7543, 7549, 7557, 7576,
    7584, 7590, 7598, 7608, 7616, 7622, 7630, 6899, 6907, 6913, 6921, 6931,
    6939, 6945, 6953, 6972, 6980, 6986, 6994, 7004, 7012, 7018, 7026, 7048,
    7056, 7062, 7070, 7080, 7088, 7094, 7102, 7121, 7129, 7135, 7143, 7153,
    7161, 7167, 7175, 7201, 7209, 7215, 7223, 7233, 7241, 7247, 7255, 7274,
    7282, 7288, 7296, 7306, 7314, 7320, 7328, 7350, 7358, 7364, 7372, 7382,
    7390, 7396, 7404, 7423, 7431, 7437, 7445, 7455, 7463, 7469, 7477, 7343,
    7351, 7357, 7365, 7375, 7383, 7389, 7397, 7416, 7424, 7430, 7438, 7448,
    7456, 7462, 7470, 7492, 7500, 7506, 7514, 7524, 7532, 7538, 7546, 7565,
    7573, 7579, 7587, 7597, 7605, 7611, 7619, 7645, 7653, 7659, 7667, 7677,
    7685, 7691, 7699, 7718, 7726, 7732, 7740, 7750, 7758, 7764, 7772, 7794,
    7802, 7808, 7816, 7826, 7834, 7840, 7848, 7867, 7875, 7881, 7889, 7899,
    7907, 7913, 7921, 7719, 7727, 7733, 7741, 7751, 7759, 7765, 7773, 7792,
    7800, 7806, 7814, 7824, 7832, 7838, 7846, 7868, 7876, 7882, 7890, 7900,
    7908, 7914, 7922, 7941, 7949, 7955, 7963, 7973, 7981, 7987, 7995, 8021,
    8029, 8035, 8043, 8053, 8061, 8067, 8075, 8094, 8102, 8108, 8116, 8126,
    8134, 8140, 8148, 8170, 8178, 8184, 8192, 8202, 8210, 8216, 8224, 8243,
    8251, 8257, 8265, 8275
};

uint16_t TokenCosts::coeff_base_cost( int16_t coeff )
{
  assert( coeff >= -2048 && coeff <= 2047 );
  return *( dct_value_cost + 2048 + coeff );
}
