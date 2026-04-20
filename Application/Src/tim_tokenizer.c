#include "tim_tokenizer.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define TIM_PAD_ID 0
#define TIM_UNK_ID 1
#define TIM_CLS_ID 2
#define TIM_SEP_ID 3
#define TIM_MAX_WORD 48U

typedef struct {
    const char *token;
    int32_t id;
} TimTokenEntry_t;

static const TimTokenEntry_t s_vocab[] = {
    {"123", 196},
    {"<c:0>", 321},
    {"<c:1>", 322},
    {"<c:2>", 323},
    {"<c:3>", 324},
    {"<c:4>", 325},
    {"<c:5>", 326},
    {"<c:6>", 327},
    {"<c:7>", 328},
    {"<c:8>", 329},
    {"<c:9>", 330},
    {"<c:a>", 295},
    {"<c:b>", 296},
    {"<c:c>", 297},
    {"<c:d>", 298},
    {"<c:e>", 299},
    {"<c:f>", 300},
    {"<c:g>", 301},
    {"<c:h>", 302},
    {"<c:i>", 303},
    {"<c:j>", 304},
    {"<c:k>", 305},
    {"<c:l>", 306},
    {"<c:m>", 307},
    {"<c:n>", 308},
    {"<c:o>", 309},
    {"<c:p>", 310},
    {"<c:q>", 311},
    {"<c:r>", 312},
    {"<c:s>", 313},
    {"<c:t>", 314},
    {"<c:u>", 315},
    {"<c:v>", 316},
    {"<c:w>", 317},
    {"<c:x>", 318},
    {"<c:y>", 319},
    {"<c:z>", 320},
    {"[CLS]", 2},
    {"[PAD]", 0},
    {"[SEP]", 3},
    {"[UNK]", 1},
    {"a", 46},
    {"abc", 191},
    {"about", 159},
    {"account", 236},
    {"activate", 68},
    {"adam", 104},
    {"adios", 205},
    {"ahead", 78},
    {"alive", 265},
    {"am", 94},
    {"an", 156},
    {"analyze", 134},
    {"any", 132},
    {"anyone", 173},
    {"anything", 42},
    {"apply", 283},
    {"appreciate", 251},
    {"appreciated", 285},
    {"are", 63},
    {"around", 164},
    {"asdf", 291},
    {"aware", 179},
    {"batery", 286},
    {"battery", 32},
    {"battrey", 210},
    {"bay", 99},
    {"be", 167},
    {"begin", 282},
    {"bella", 103},
    {"blah", 182},
    {"board", 39},
    {"book", 208},
    {"bored", 189},
    {"bottles", 80},
    {"by", 228},
    {"bye", 124},
    {"byee", 198},
    {"call", 116},
    {"camera", 10},
    {"camra", 150},
    {"can", 53},
    {"cancel", 165},
    {"cars", 82},
    {"chairs", 81},
    {"check", 5},
    {"cheering", 258},
    {"clever", 136},
    {"command", 131},
    {"commands", 143},
    {"communicating", 237},
    {"comprendo", 280},
    {"condition", 21},
    {"confirm", 186},
    {"conscious", 147},
    {"could", 215},
    {"cpu", 40},
    {"dd", 222},
    {"deactivate", 67},
    {"describe", 157},
    {"detct", 220},
    {"detect", 71},
    {"detected", 142},
    {"detection", 24},
    {"device", 12},
    {"diagnostics", 19},
    {"disable", 62},
    {"do", 47},
    {"doing", 138},
    {"door", 100},
    {"enable", 66},
    {"enough", 229},
    {"essay", 193},
    {"execute", 221},
    {"exit", 219},
    {"factory", 247},
    {"ff", 232},
    {"find", 75},
    {"firmware", 43},
    {"for", 16},
    {"forgot", 194},
    {"free", 248},
    {"friends", 217},
    {"front", 149},
    {"fuck", 163},
    {"genious", 166},
    {"get", 214},
    {"girl", 137},
    {"give", 88},
    {"good", 112},
    {"goodbye", 121},
    {"gossip", 109},
    {"got", 212},
    {"great", 122},
    {"halt", 92},
    {"hardware", 36},
    {"have", 146},
    {"health", 23},
    {"hear", 246},
    {"hello", 126},
    {"helloo", 224},
    {"helo", 262},
    {"help", 120},
    {"helpful", 278},
    {"hey", 231},
    {"hi", 153},
    {"hii", 281},
    {"hiii", 230},
    {"hiya", 242},
    {"hola", 176},
    {"hope", 161},
    {"hours", 238},
    {"how", 101},
    {"human", 170},
    {"hya", 158},
    {"i", 76},
    {"identify", 73},
    {"in", 113},
    {"inference", 49},
    {"info", 27},
    {"information", 17},
    {"inspect", 154},
    {"intelligent", 162},
    {"is", 25},
    {"it", 107},
    {"items", 87},
    {"jj", 268},
    {"joke", 133},
    {"jokes", 275},
    {"know", 115},
    {"later", 233},
    {"laugh", 270},
    {"level", 20},
    {"list", 169},
    {"load", 18},
    {"locate", 77},
    {"look", 64},
    {"lot", 225},
    {"made", 261},
    {"make", 269},
    {"me", 26},
    {"mean", 243},
    {"meant", 245},
    {"memory", 35},
    {"memry", 263},
    {"method", 274},
    {"model", 13},
    {"more", 174},
    {"much", 284},
    {"music", 241},
    {"my", 93},
    {"name", 96},
    {"nearby", 79},
    {"need", 177},
    {"night", 264},
    {"not", 106},
    {"objcts", 290},
    {"object", 50},
    {"objects", 72},
    {"obstacle", 218},
    {"obstacles", 85},
    {"of", 254},
    {"off", 65},
    {"ok", 148},
    {"okay", 181},
    {"on", 60},
    {"online", 267},
    {"open", 97},
    {"order", 250},
    {"package", 292},
    {"password", 195},
    {"pause", 69},
    {"payment", 273},
    {"people", 86},
    {"percentage", 199},
    {"person", 84},
    {"pipeline", 48},
    {"play", 240},
    {"please", 4},
    {"pod", 98},
    {"power", 41},
    {"processor", 38},
    {"prove", 118},
    {"quiet", 140},
    {"quit", 187},
    {"qwerty", 257},
    {"ram", 33},
    {"read", 244},
    {"real", 110},
    {"rebboot", 203},
    {"reboot", 58},
    {"recognize", 74},
    {"report", 6},
    {"reset", 54},
    {"resett", 289},
    {"restar", 201},
    {"restart", 57},
    {"resume", 70},
    {"return", 249},
    {"room", 239},
    {"run", 91},
    {"running", 200},
    {"s", 152},
    {"save", 253},
    {"saying", 171},
    {"scan", 59},
    {"scanning", 51},
    {"scene", 135},
    {"see", 44},
    {"sees", 211},
    {"self", 178},
    {"sensor", 37},
    {"sensors", 14},
    {"services", 279},
    {"settings", 151},
    {"shhh", 190},
    {"shit", 255},
    {"show", 7},
    {"shut", 294},
    {"shutdown", 56},
    {"sing", 259},
    {"soft", 207},
    {"some", 144},
    {"song", 260},
    {"speaking", 288},
    {"stable", 168},
    {"start", 61},
    {"stat", 202},
    {"state", 22},
    {"status", 8},
    {"stop", 55},
    {"stopp", 197},
    {"strt", 293},
    {"subscription", 206},
    {"support", 227},
    {"surely", 266},
    {"surroundings", 145},
    {"system", 11},
    {"t", 130},
    {"tables", 83},
    {"talking", 111},
    {"taxi", 209},
    {"technical", 226},
    {"telemetry", 15},
    {"tell", 45},
    {"temperatur", 216},
    {"temperature", 34},
    {"thank", 127},
    {"thanks", 89},
    {"thankyou", 252},
    {"that", 277},
    {"the", 31},
    {"there", 90},
    {"think", 183},
    {"this", 114},
    {"through", 235},
    {"thx", 271},
    {"time", 105},
    {"to", 102},
    {"turn", 30},
    {"twat", 256},
    {"u", 188},
    {"understand", 175},
    {"up", 185},
    {"update", 272},
    {"usag", 204},
    {"usage", 28},
    {"use", 160},
    {"user", 123},
    {"very", 125},
    {"view", 119},
    {"visible", 180},
    {"vision", 52},
    {"want", 184},
    {"was", 223},
    {"wasn", 129},
    {"weather", 287},
    {"well", 139},
    {"what", 29},
    {"where", 172},
    {"who", 117},
    {"why", 108},
    {"will", 141},
    {"with", 155},
    {"working", 128},
    {"write", 192},
    {"yo", 276},
    {"you", 9},
    {"your", 95},
    {"youtube", 213},
    {"yu", 234},
};

static int32_t VocabLookup(const char *token)
{
    for (uint32_t i = 0U; i < (uint32_t)(sizeof(s_vocab) / sizeof(s_vocab[0])); i++)
    {
        if (strcmp(token, s_vocab[i].token) == 0)
        {
            return s_vocab[i].id;
        }
    }
    return TIM_UNK_ID;
}

static int32_t CharTokenId(char c)
{
    char token[6] = {'<', 'c', ':', c, '>', '\0'};
    return VocabLookup(token);
}

static char NormalizeChar(char c)
{
    if ((c >= 'A') && (c <= 'Z'))
    {
        return (char)(c + ('a' - 'A'));
    }
    if (((c >= 'a') && (c <= 'z')) || ((c >= '0') && (c <= '9')))
    {
        return c;
    }
    return ' ';
}

static void EmitWord(const char *word, uint32_t word_len, int32_t input_ids[TIM_TOKENIZER_MAX_LEN], uint32_t *pos)
{
    char buf[TIM_MAX_WORD];
    uint32_t n = word_len;
    if (n >= TIM_MAX_WORD)
    {
        n = TIM_MAX_WORD - 1U;
    }
    memcpy(buf, word, n);
    buf[n] = '\0';

    int32_t id = VocabLookup(buf);
    if (id != TIM_UNK_ID)
    {
        if (*pos < (TIM_TOKENIZER_MAX_LEN - 1U))
        {
            input_ids[(*pos)++] = id;
        }
        return;
    }

    for (uint32_t i = 0U; (i < n) && (*pos < (TIM_TOKENIZER_MAX_LEN - 1U)); i++)
    {
        int32_t cid = CharTokenId(buf[i]);
        input_ids[(*pos)++] = cid;
    }
}

void TIM_Tokenizer_Encode(const char *text, int32_t input_ids[TIM_TOKENIZER_MAX_LEN])
{
    for (uint32_t i = 0U; i < TIM_TOKENIZER_MAX_LEN; i++)
    {
        input_ids[i] = TIM_PAD_ID;
    }

    uint32_t pos = 0U;
    input_ids[pos++] = TIM_CLS_ID;

    if (text != NULL)
    {
        char word[TIM_MAX_WORD];
        uint32_t word_len = 0U;

        for (const char *p = text; ; p++)
        {
            char c = NormalizeChar(*p);
            if ((c != ' ') && (*p != '\0'))
            {
                if (word_len < (TIM_MAX_WORD - 1U))
                {
                    word[word_len++] = c;
                }
            }
            else
            {
                if (word_len > 0U)
                {
                    EmitWord(word, word_len, input_ids, &pos);
                    word_len = 0U;
                }
                if ((*p == '\0') || (pos >= (TIM_TOKENIZER_MAX_LEN - 1U)))
                {
                    break;
                }
            }
        }
    }

    if (pos < TIM_TOKENIZER_MAX_LEN)
    {
        input_ids[pos] = TIM_SEP_ID;
    }
    else
    {
        input_ids[TIM_TOKENIZER_MAX_LEN - 1U] = TIM_SEP_ID;
    }
}
