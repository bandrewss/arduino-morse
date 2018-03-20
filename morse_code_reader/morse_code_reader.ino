// Ben Andrews
// PHYS 333 Morse Code Reader
// 2018-3-18

#define CLOCK_IN 12
#define MORSE_IN 13

#define DOT ('.')
#define DASH ('-')
#define WORD_SPACE ('W')
#define LETTER_SPACE ('L')
#define END_STRING ('E')
#define ERR ('R')

// Hashes:
//  DOT: 1
//  DASH: 0
//  END: 1
#define MORSE_HASH_A 0b00000110
#define MORSE_HASH_B 0b00010111
#define MORSE_HASH_C 0b00010101
#define MORSE_HASH_D 0b00001011
#define MORSE_HASH_E 0b00000011
#define MORSE_HASH_F 0b00011101
#define MORSE_HASH_G 0b00001001
#define MORSE_HASH_H 0b00011111
#define MORSE_HASH_I 0b00000111
#define MORSE_HASH_J 0b00011000
#define MORSE_HASH_K 0b00001010
#define MORSE_HASH_L 0b00011011
#define MORSE_HASH_M 0b00000100
#define MORSE_HASH_N 0b00000101
#define MORSE_HASH_O 0b00001000
#define MORSE_HASH_P 0b00011001
#define MORSE_HASH_Q 0b00010010
#define MORSE_HASH_R 0b00001101
#define MORSE_HASH_S 0b00001111
#define MORSE_HASH_T 0b00000010
#define MORSE_HASH_U 0b00001110
#define MORSE_HASH_V 0b00011110
#define MORSE_HASH_W 0b00001100
#define MORSE_HASH_X 0b00010110
#define MORSE_HASH_Y 0b00010100
#define MORSE_HASH_Z 0b00010011
#define MORSE_HASH_1 0b00110000
#define MORSE_HASH_2 0b00111000
#define MORSE_HASH_3 0b00111100
#define MORSE_HASH_4 0b00111110
#define MORSE_HASH_5 0b00111111
#define MORSE_HASH_6 0b00101111
#define MORSE_HASH_7 0b00100111
#define MORSE_HASH_8 0b00100011
#define MORSE_HASH_9 0b00100001
#define MORSE_HASH_0 0b00100000
#define MORSE_HASH_SPACE 0b00000000
#define MORSE_HASH_ERROR 0b11111111
#define MORSE_HASH_EOS 0b11000011

#define BUF_SIZE 1024

char clock_state, clock_looking;
int low_count = 0;

char buf[BUF_SIZE];
int buf_p = 0;

void setup() 
{
    Serial.begin(9600);
    while(!Serial)
        ;
    
    pinMode(CLOCK_IN, INPUT);
    pinMode(MORSE_IN, INPUT);

    clock_looking = HIGH;
}

void loop() 
{
loop:
    clock_state = digitalRead(CLOCK_IN);
    
    if(clock_state == clock_looking)
    {
        clock_looking = !clock_looking;

        if(clock_state == HIGH)
        {
            // reuse clock_state
            clock_state = digitalRead(MORSE_IN);

            // always start the buffer on a high read
            if(buf_p == 0 && clock_state == LOW)
                goto loop;

            buf[buf_p++] = clock_state;

            Serial.println(clock_state, DEC);

            if(buf_p >= BUF_SIZE)
            {
                Serial.println("BUFFER OVERFLOW, DECODING PREMATURELY");
                goto decode;
            }

            if(clock_state == LOW)
            {
                ++low_count;

                // end of string is determined when 8 clock cycles have gone without a high
                if(low_count >= 8)
                {
                decode:
                    Serial.println("Clock Buffer: ");
                    print_buf();
                    Serial.println("Decode Clock: ");
                    decode_clock();
                    Serial.println("Decode Morse: ");
                    decode_morse();
                    buf_p = 0;
                }
            }
            else
            {
                low_count = 0;
            }
        }
    }    
}


// print the raw buffer in an easy to read manner
void print_buf()
{
    Serial.print("Buffer size: ");
    Serial.println(buf_p);
    
    for(int i = 0; i < buf_p; i += 16)
    {
        
        for(int j = 0; j < 16 && j+i < buf_p; ++j)
            Serial.print(buf[i + j], DEC);
            //Serial.println(i+j);

        Serial.println();
    }
}


// turn the clock signals into morse
void decode_clock()
{
    int data_p = 0;
    int data_f = 0;
    int low_space;
    
    int morse_p = 0;

    // while we are reading valid data
    while(data_p < buf_p)
    {        
        if(buf[data_p] == HIGH)
        {   
            // non elegant dot/dash finder
            if(buf[data_p +1] == LOW)
            {
                buf[morse_p] = DOT;
                ++data_p;
            }
            else if(buf[data_p +1] == HIGH && buf[data_p +2] == HIGH)
            {
                buf[morse_p] = DASH;
                data_p += 3;
            }
            else
            {
                buf[morse_p] == ERR;
                data_p += 1;
            }
        }
        else
        {
            data_f = data_p;

            // find the next high
            while(buf[++data_f] == LOW)
                ;

            low_space = data_f - data_p;
                
            if(low_space == 1)
                morse_p--;
            else if(low_space == 3)
                buf[morse_p] = LETTER_SPACE;
            else if(low_space == 7)
                buf[morse_p] = WORD_SPACE;
            else if(low_space > 7)
                buf[morse_p] = END_STRING;
            else
                buf[morse_p] = ERR;

            data_p = data_f;
        }

        ++morse_p;
    }

    for(int i = 0; i < morse_p; ++i)
        Serial.write(buf[i]);

    Serial.println();

    // reset the end of buffer pointer to the end of the morse
    buf_p = morse_p;
}


// turn the morse into letters
void decode_morse()
{    
    int decode_p = 0; // where to put the letters

    int letter_start = 0;
    int letter_end = 0;
    
    char morse_buf[8];
    int morse_p = 0;

    byte morse_hash;
    
    while(letter_start < buf_p)
    {
        switch(buf[letter_start])
        {
            case WORD_SPACE:
                morse_hash = MORSE_HASH_SPACE;
                break;
            case ERR:
                morse_hash = MORSE_HASH_ERROR;
                break;
            case END_STRING:
                morse_hash = MORSE_HASH_EOS;
                break;
            case LETTER_SPACE:
                ++letter_start;
            case DOT:
            case DASH:
                letter_end = letter_start;
                morse_p = 0;

                // get all the dots and dashes that create a letter
                while(buf[letter_end] != LETTER_SPACE 
                   && buf[letter_end] != WORD_SPACE 
                   && buf[letter_end] != END_STRING
                   && buf[letter_end] != ERR
                   && morse_p < 7)
                {         
                    morse_buf[morse_p++] = buf[letter_end++];
                }
    
                morse_buf[morse_p] = '\0';
               
                // create hash from dots and dash
                //  scheme described above
                morse_hash = 1;
                
                for(int i = 0; i < morse_p; ++i)
                {                
                    morse_hash <<= 1;
                    if(morse_buf[i] == DOT)
                        ++morse_hash;
                }
    
                letter_start = letter_end;
               
                goto skip;
        }

        ++letter_start;
    skip:

        // switch on the letter hash to insert the correct letter
        switch(morse_hash)
        {
            case MORSE_HASH_A:
                buf[decode_p] = 'A';
                break;
            case MORSE_HASH_B:
                buf[decode_p] = 'B'; 
                break;
            case MORSE_HASH_C:
                buf[decode_p] = 'C';
                break;
            case MORSE_HASH_D:
                buf[decode_p] = 'D'; 
                break;
            case MORSE_HASH_E:
                buf[decode_p] = 'E'; 
                break;
            case MORSE_HASH_F:
                buf[decode_p] = 'F'; 
                break;
            case MORSE_HASH_G:
                buf[decode_p] = 'G'; 
                break;
            case MORSE_HASH_H:
                buf[decode_p] = 'H'; 
                break;
            case MORSE_HASH_I:
                buf[decode_p] = 'I'; 
                break;
            case MORSE_HASH_J:
                buf[decode_p] = 'J'; 
                break;
            case MORSE_HASH_K:
                buf[decode_p] = 'K'; 
                break;
            case MORSE_HASH_L:
                buf[decode_p] = 'L';
                break;
            case MORSE_HASH_M:
                buf[decode_p] = 'M'; 
                break;
            case MORSE_HASH_N:
                buf[decode_p] = 'N'; 
                break;
            case MORSE_HASH_O:
                buf[decode_p] = '0';
                break;
            case MORSE_HASH_P:
                buf[decode_p] = 'P';
                break;
            case MORSE_HASH_Q:
                buf[decode_p] = 'Q';
                break;
            case MORSE_HASH_R:
                buf[decode_p] = 'R'; 
                break;
            case MORSE_HASH_S:
                buf[decode_p] = 'S';
                break;
            case MORSE_HASH_T:
                buf[decode_p] = 'T'; 
                break;
            case MORSE_HASH_U:
                buf[decode_p] = 'U'; 
                break;
            case MORSE_HASH_V:
                buf[decode_p] = 'V'; 
                break;
            case MORSE_HASH_W:
                buf[decode_p] = 'W'; 
                break;
            case MORSE_HASH_X:
                buf[decode_p] = 'X'; 
                break;
            case MORSE_HASH_Y:
                buf[decode_p] = 'Y'; 
                break;
            case MORSE_HASH_Z:
                buf[decode_p] = 'Z';
                break;
            case MORSE_HASH_1:
                buf[decode_p] = '1';
                break;
            case MORSE_HASH_2:
                buf[decode_p] = '2';
                break;
            case MORSE_HASH_3:
                buf[decode_p] = '3';
                break;
            case MORSE_HASH_4:
                buf[decode_p] = '4';
                break;
            case MORSE_HASH_5:
                buf[decode_p] = '5';
                break;
            case MORSE_HASH_6:
                buf[decode_p] = '6';
                break;
            case MORSE_HASH_7:
                buf[decode_p] = '7';
                break;
            case MORSE_HASH_8:
                buf[decode_p] = '8';
                break;
            case MORSE_HASH_9:
                buf[decode_p] = '9';
                break;
            case MORSE_HASH_0:
                buf[decode_p] = '0';
                break;
            case MORSE_HASH_SPACE:
                buf[decode_p] = ' ';
                break;
            case MORSE_HASH_ERROR:
                buf[decode_p] = '_';
                break;
            case MORSE_HASH_EOS:
                buf[decode_p++] = ' ';
                buf[decode_p++] = ':';
                buf[decode_p++] = 'E';
                buf[decode_p++] = 'N';
                buf[decode_p++] = 'D';
                buf[decode_p] = ':';
                
                break;
            default:
                buf[decode_p] = '#';
                
        }

        ++decode_p;
    }
    
    buf[decode_p] = '\0';
    Serial.println(buf);
    
    Serial.println('\0');
}













