#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

#define FREQ16M
#ifdef FREQ64M
#define _XTAL_FREQ 64000000
#endif
#ifdef FREQ16M
#define _XTAL_FREQ 16000000
#endif
#ifdef FREQ32M
#define _XTAL_FREQ 32000000
#endif
#define BYTE unsigned char

#define MAX_METEOR 8

//ピン配置
#define SHIFT_REG_CLK PORTBbits.RB5
#define SHIFT_REG_DAT PORTBbits.RB4
#define MAT_ROW_RED PORTA
#define MAT_ROW_GRN PORTC
#define SW_UP LATB0
#define SW_LEFT LATB1
#define SW_RIGHT LATB2
#define SW_DOWN LATB3

#define UNEXIST 0
#define UP_V 1
#define RIGHT_V 2
#define DOWN_V 3
#define LEFT_V 4

#define POWERON 0
#define CNT_THREE 1
#define CNT_TWO 2
#define CNT_ONE 3
#define CNT_GO 4
#define PLAYING 5
#define FAIL 6
#define GAMEOVER 7
#define LAST 8
#define CLEAR 9

#pragma config FOSC = INTIO67,PLLCFG = ON,BOREN=ON,BORV=190,PWRTEN=ON,WDTEN=OFF,PBADEN=OFF,MCLRE=EXTMCLR

// 変数たち
struct Player{
    BYTE pos_x;
    BYTE pos_y;
};
struct Object{
    BYTE pos_x;
    BYTE pos_y;
    BYTE vector;
};
struct Player You;
struct Object Meteor[MAX_METEOR];
BYTE music_freq = 50;
BYTE play_music = 0;
BYTE sequence;
unsigned int SystemTimer=0;
unsigned int score_count=0;
BYTE high_10 = 0;
BYTE high_1 = 0;
#ifdef FREQ64M
const BYTE diff_speed[] = {80,60,50,40,30,25,20,15};
#endif
#ifdef FREQ16M
const unsigned int diff_speed[] = {500,460,420,380,340,300,300};
#endif
#ifdef FREQ32M
const unsigned int diff_speed[] = {500,450,400,350,330,315,300};
#endif
//テーブルなどのデータ
const BYTE table_one[]={
    0b00000000,
    0b00000000,
    0b00000000,
    0b10000010,
    0b11111111,
    0b10000000,
    0b00000000,
    0b00000000,
};
const BYTE table_two[]={
    0b00000000,
    0b00000000,
    0b10001100,
    0b11000010,
    0b10100010,
    0b10011100,
    0b00000000,
    0b00000000
};
const BYTE table_three[]={
    0b00000000,
    0b00000000,
    0b01000010,
    0b10001001,
    0b10001001,
    0b01110110,
    0b00000000,
    0b00000000,
};
const BYTE table_GO[]={
    0b00011110,
    0b00100001,
    0b00101001,
    0b00011010,
    0b01110000,
    0b10001000,
    0b10001000,
    0b01110000
};
const BYTE table_FIN[]={
    0b00000000,
    0b00000000,
    0b00111100,
    0b01000010,
    0b01010010,
    0b00110100,
    0b00000000,
    0b00000000
};
const BYTE table_HI[]={
    0b00000000,
    0b00000111,
    0b00000010,
    0b00000111,
    0b00000000,
    0b00000101,
    0b00000111,
    0b00000101
};
const BYTE number[10][3]={{
        0b11111000,
        0b10001000,
        0b11111000
    },{
        0b00000000,
        0b11111000,
        0b00000000
    },{
        0b11101000,
        0b10101000,
        0b10111000
    },{
        0b10101000,
        0b10101000,
        0b11111000
    },{
        0b00111000,
        0b00100000,
        0b11111000
    },{
        0b10111000,
        0b10101000,
        0b11101000
    },{
        0b11111000,
        0b10101000,
        0b11101000
    },{
        0b00111000,
        0b00001000,
        0b11111000
    },{
        0b11111000,
        0b10101000,
        0b11111000
    },{
        0b10111000,
        0b10101000,
        0b11111000
    }
};
BYTE Stage_r[8]={
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
};
BYTE Stage_g[8]={
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
};

/*
 * レジスタ設定・シフトレジスタ初期化など
 */
void Initialization(){
    BYTE i;
    //初期設定
#ifdef FREQ64M
    OSCCON = 0b01110000;    //64MHz動作
    OSCCON2bits.PLLRDY = 1;
    OSCTUNEbits.PLLEN = 1;
#endif
#ifdef FREQ16M
    OSCCON = 0b01110010;    //16MHz動作
#endif
#ifdef FREQ32M
    OSCCON = 0b01100000;    //8MHz
    OSCCON2bits.PLLRDY = 1;
    OSCTUNEbits.PLLEN = 1;
#endif
    ANSELA = 0x00;          //ADCは使わない
    ANSELB = 0x00;
    ANSELC = 0x00;
    TRISA = 0x00;           //全部出力
    TRISB = 0x0F;           //RB3~RB0は入力
    TRISC = 0x00;           //全部出力
    PORTA = 0x00;           //出力リセット
    PORTB = 0x00;
    PORTC = 0x00;
    WPUB = 0x0F;            //RB3~RB0はプルアップ
    INTCON2bits.RBPU = 0;        //プルアップ有効

    INTCON3bits.INT2IF = 0;     //RB2割込みフラグクリア
    INTCON2bits.INTEDG2 = 0;    //rising edge
    INTCON3bits.INT2IP = 0;     //low priority
    INTCON3bits.INT2IE = 0;     //RB2割り込み禁止
    /*INTCON3bits.INT1IF = 0;     //RB2割込みフラグクリア
    INTCON2bits.INTEDG1 = 0;    //rising edge
    INTCON3bits.INT1IP = 0;     //low priority
    INTCON3bits.INT1IE = 0;     //RB2割り込み禁止
    INTCONbits.INT0IF = 0;     //RB2割込みフラグクリア
    INTCON2bits.INTEDG0 = 0;    //rising edge
    //INTCON2bits.INT0IP = 0;     //low priority
    INTCONbits.INT0IE = 0;     //RB2割り込み禁止*/
    INTCONbits.PEIE = 1;        //ペリフェラル割り込み有効
/*
    INTCONbits.RBIF = 0;
    INTCONbits.RBIE = 1;
*/
    //シフトレジスタ初期化
    SHIFT_REG_DAT = 1;
    for(i=0;i<8;i++){
        SHIFT_REG_CLK=1;
        __delay_us(1);
        SHIFT_REG_CLK=0;
        __delay_us(1);
    }
}
void Meteor_gen(BYTE num,BYTE pos,BYTE vec){
    pos &= 0x07;
    vec &= 0x03;
    switch(vec){
        case 0:
            Meteor[num].pos_x = pos;
            Meteor[num].pos_y = 7;
            Meteor[num].vector = DOWN_V;
            break;
        case 1:
            Meteor[num].pos_y = pos;
            Meteor[num].pos_x = 0;
            Meteor[num].vector = RIGHT_V;
            break;
        case 2:
            Meteor[num].pos_y = pos;
            Meteor[num].pos_x = 7;
            Meteor[num].vector = LEFT_V;
            break;
        case 3:
            Meteor[num].pos_x = pos;
            Meteor[num].pos_y = 0;
            Meteor[num].vector = UP_V;
            break;
        default:
            break;
    }
}
void Meteor_move(){
    BYTE i=0;
    music_freq = 10;
    play_music = 1;
    for(i=0;i<MAX_METEOR;i++){
        if(Meteor[i].vector != UNEXIST){
            if(Meteor[i].vector == RIGHT_V){
                Meteor[i].pos_x ++;
                //if(Meteor[i].pos_x == 8)    Meteor[i].pos_x = 7;
            }
            if(Meteor[i].vector == UP_V){
                Meteor[i].pos_y ++;
                //if(Meteor[i].pos_y == 8)    Meteor[i].pos_y = 7;
            }
            if(Meteor[i].vector == DOWN_V){
                Meteor[i].pos_y --;
                //if(Meteor[i].pos_y == 0xFF)    Meteor[i].pos_y = 0;
            }
            if(Meteor[i].vector == LEFT_V){
                Meteor[i].pos_x --;
                //if(Meteor[i].pos_x == 0xFF)    Meteor[i].pos_x = 0;
            }
            if(Meteor[i].pos_y == 0xFF || Meteor[i].pos_y == 8 || Meteor[i].pos_x == 0xFF || Meteor[i].pos_x == 8){
                Meteor[i].vector=UNEXIST;
            }
        }
    }
    __delay_ms(10);
    play_music = 0;
}
BYTE Collide_check(){
    BYTE i;
    for(i=0;i<MAX_METEOR;i++){
        if(Meteor[i].vector != UNEXIST){
            if(Meteor[i].pos_x == You.pos_x && Meteor[i].pos_y == You.pos_y){
                return 1;
            }
        }
    }
    return 0;
}
void Key_check(){
    static BYTE key_check=0;
    if(!SW_UP){
        if((key_check & 0x01) == 0){
            You.pos_y++;
            if(You.pos_y == 8)  You.pos_y--;
            key_check |= 0x01;
        }
    }else{
        key_check &= 0xFE;
    }
    if(!SW_RIGHT){
        if((key_check & 0x02) == 0){
            You.pos_x++;
            if(You.pos_x == 8)  You.pos_x--;
            key_check |= 0x02;
        }
    }else{
        key_check &= 0xFD;
    }
    if(!SW_DOWN){
        if((key_check & 0x04) == 0){
            You.pos_y--;
            if(You.pos_y == 0xFF)    You.pos_y++;
            key_check |= 0x04;
        }
    }else{
        key_check &= 0xFB;
    }
    if(!SW_LEFT){
        if((key_check & 0x08) == 0){
            You.pos_x--;
            if(You.pos_x == 0xFF)    You.pos_x ++;
            key_check |= 0x08;
        }
    }else{
        key_check &= 0xF7;
    }
}

void Stage_draw(){
    BYTE i;
        for(i=0;i<8;i++){
            Stage_g[i]=0x00;
            Stage_r[i]=0x00;
        }
        Stage_g[You.pos_x] |= (0x80 >> You.pos_y);
        for(i=0;i<MAX_METEOR;i++){
            if(Meteor[i].vector != UNEXIST){
                Stage_r[Meteor[i].pos_x] |= (0x80 >> Meteor[i].pos_y);
            }
        }
}
/*
 * 割り込み設定（タイマー0）
 */
void Interrupt_start(){
#ifdef FREQ64M
    T0CON = 0b11000111;
    TMR0 = 6;           
#endif
#ifdef FREQ16M
    //20kHz
    TMR0 = 206;         //256-50 = 206
    T0CON = 0b11000000;
#endif
#ifdef FREQ32M
    TMR0 = 156;
    T0CON = 0b11000010; //20kHz割り込み
#endif
    T0IF = 0;           //割り込みフラグ初期化
    T0IE = 1;           //タイマー0割り込み許可
    GIE = 1;            //全体割り込み許可
}
#define SECOND_100msec 50
void interrupt IntTimer0(){
    //ステージ描画
    static BYTE i=0;
    static BYTE counter = 0;
    static BYTE music_counter=0;
    if(i==0){
        SHIFT_REG_DAT = 0;
        i++;
    }
    if(i>0){
        MAT_ROW_RED = 0x00;
        MAT_ROW_GRN = 0x00;
        //__delay_us(1);
        SHIFT_REG_CLK = 1;
        MAT_ROW_RED = Stage_r[i-1];
        MAT_ROW_GRN = Stage_g[i-1];
        //__delay_us(DYNAMIC_TIME);
        SHIFT_REG_DAT = 1;
        __delay_us(1);
        SHIFT_REG_CLK = 0;
    }
    i++;
    if(i==9){
        i=0;
    }
    if(counter == 20){
        score_count ++;
        SystemTimer++;
        counter = 0;
        if(sequence == PLAYING){
            Key_check();
            //score_count ++;
        }
    }
    counter ++;
    if(play_music && music_counter == music_freq){
        PORTBbits.RB6 ^= 1;
        music_counter = 0;
    }
    music_counter ++;
    if (T0IF == 1) {    //タイマー0の割込み発生か？
        TMR0 = 206;       //タイマー0の再度初期化
        T0IF = 0;       //タイマー0割込フラグをリセット(再カウントアップ開始)
    }
}
void HighScore(){
    BYTE i;
    //GIE = 0;    //割り込みはさせない
    music_freq = 15;
    play_music = 1;
    __delay_ms(40);
    music_freq = 13;
    __delay_ms(40);
    music_freq = 8;
    __delay_ms(40);
    music_freq = 10;
    __delay_ms(40);
    music_freq = 9;
    __delay_ms(40);
    music_freq = 5;
    __delay_ms(40);
    play_music = 0;
    //GIE = 1;
}
void Clear(){
    BYTE i;
    music_freq = 10;
    play_music = 1;
    for(i=0;i<10;i++)__delay_ms(15);
    music_freq = 11;
    for(i=0;i<10;i++)__delay_ms(6);
    music_freq = 10;
    for(i=0;i<10;i++)   __delay_ms(30);
    play_music = 0;
}
void Miss(){
    BYTE i;
    music_freq = 40;
    play_music = 1;
    for(i=0;i<10;i++)   __delay_ms(20);
    play_music = 0;
}
void Score(){
    BYTE i;
    music_freq = 10;
    play_music = 1;
    for(i=0;i<20;i++)  __delay_ms(10);
    play_music = 0;
}
void main(){
    //変数たち
    BYTE i;
    BYTE counter=0;  //初期化はここで
    unsigned int gen_counter = 0;
    BYTE num_counter = 0;
    BYTE score_10=0;
    BYTE score_1=0;
    unsigned int  meteor_move_time;
    BYTE HARDMODE = 0;

    Initialization();
    sequence = POWERON;

    Interrupt_start();

    while(1){
        counter ++;
        if(!SW_UP || !SW_DOWN || !SW_RIGHT || !SW_LEFT){
            srand(counter);
            counter = 0;
            break;
        }
    }
    while(1){
        if(sequence==POWERON){
            HARDMODE = 0;
            score_1 = 0;
            score_10 = 0;
            gen_counter=0;
            num_counter = 0;
            score_count=0;  //グローバル
            meteor_move_time=0;
            for(i=0;i<MAX_METEOR;i++){
                Meteor[i].vector=UNEXIST;
            }
            Stage_r[0]=0x00;
            Stage_r[1]=number[high_10][0];
            Stage_r[2]=number[high_10][1];
            Stage_r[3]=number[high_10][2];
            Stage_r[4]=0x00;
            Stage_r[5]=number[high_1][0];
            Stage_r[6]=number[high_1][1];
            Stage_r[7]=number[high_1][2];
            for(i=0;i<8;i++){
                Stage_g[i]=table_HI[i];
            }
            if(SystemTimer >= SECOND_100msec * 200){
                for(i=0;i<8;i++){
                    Stage_g[i] = 0x00;
                    Stage_r[i] = 0x00;
                }
                __delay_ms(10);
                INTCON3bits.INT2E = 1;
                INTCONbits.GIE = 0;
                INTCON3bits.INT2IF = 0;
                SLEEP();
                NOP();
                INTCON3bits.INT2IF = 0;
                INTCON3bits.INT2E = 0;
                INTCONbits.GIE = 1;
                SystemTimer = 0;
                music_freq = 10;
                play_music = 1;
                __delay_ms(40);
                play_music = 0;
            }
            if(SW_UP && SW_DOWN && SW_RIGHT && SW_LEFT){
                counter = 1;
            }else if((SystemTimer >= SECOND_100msec*5) && counter && (!SW_UP || !SW_DOWN || !SW_RIGHT || !SW_LEFT)){
                counter = 0;
                SystemTimer = 0;
                sequence = CNT_THREE;
            }
        }else if(sequence == CNT_THREE){
            music_freq = 10;
            play_music = 1;
            for(i=0;i<8;i++){
                Stage_g[i] = 0x00;
                Stage_r[i] = table_three[i];
            }
            __delay_ms(40);
            play_music = 0;
            You.pos_x=3;
            You.pos_y=3;
            meteor_move_time = diff_speed[score_10];
            //counter++;
            for(i=0;i<40;i++)   __delay_ms(6);
            sequence = CNT_TWO;
        }else if(sequence == CNT_TWO){
            for(i=0;i<8;i++){
                Stage_r[i] = table_two[i];
            }
            if(!SW_UP || !SW_RIGHT || !SW_DOWN || !SW_LEFT )    HARDMODE ++;
            play_music = 1;
            __delay_ms(40);
            play_music = 0;
            for(i=0;i<40;i++)   __delay_ms(6);
            sequence = CNT_ONE;
        }else if(sequence == CNT_ONE){
            for(i=0;i<8;i++){
                Stage_r[i] = table_one[i];
            }
            if(!SW_UP || !SW_RIGHT || !SW_DOWN || !SW_LEFT )    HARDMODE ++;
            play_music = 1;
            __delay_ms(40);
            play_music = 0;
            for(i=0;i<40;i++)   __delay_ms(6);
            sequence = CNT_GO;
        }else if(sequence == CNT_GO){
            for(i=0;i<8;i++){
                Stage_r[i] = table_GO[i];
            }
            music_freq = 5;
            play_music = 1;
            __delay_ms(40);
            for(i=0;i<40;i++)   __delay_ms(6);
            play_music = 0;
            sequence = PLAYING;
            SystemTimer = 0;
            gen_counter = 0;
        }else if(sequence==PLAYING){
            //隕石移動処理
            if(SystemTimer >= meteor_move_time){
                Meteor_move();
                SystemTimer = 0;
                //隕石生成処理
                if(HARDMODE >= 1){
                    Meteor_gen(num_counter,rand(),gen_counter);
                    num_counter ++;
                    if(num_counter == MAX_METEOR)       num_counter = 0;
                }else{
                    if((gen_counter & 0x01) != 0x00){
                        Meteor_gen(num_counter,rand(),(gen_counter >> 1));
                        num_counter ++;
                        if(num_counter == MAX_METEOR)       num_counter = 0;
                    }
                }
                gen_counter++;
            }
            if(HARDMODE==2){
                meteor_move_time = (diff_speed[score_10] >> 1);
            }else{
                meteor_move_time = diff_speed[score_10];
            }
            //score_countは割り込みの方でカウントされている
            if(score_count >= SECOND_100msec * 20){
                score_count = 0;
                score_1 ++;
            }
            if(score_1 == 10){
                counter = 0;
                score_1 = 0;
                score_10 ++;
                gen_counter=0;
            }
            if(score_10 == 6){
                sequence = LAST;
                counter = 0;
                score_count = 0;
            }
            if(Collide_check()){
                sequence = FAIL;
                __delay_ms(10);
                counter=0;
                score_count = 0;
                SystemTimer = 0;
            }
            Stage_draw();
        }else if(sequence == LAST){
            Key_check();
            if(SystemTimer >= meteor_move_time){
                Meteor_move();
                SystemTimer = 0;
                counter ++;
            }
            Stage_draw();
            if(counter == 16){
                sequence = CLEAR;
                SystemTimer = 0;
                counter = 0;
                for(i=0;i<8;i++){
                    Stage_g[i] = 0x00;
                    Stage_r[i] = table_FIN[i];
                }
                Clear();
                high_1 = 0;
                high_10 = 6;
            }
        }else if(sequence == CLEAR){
            if(!SW_UP || !SW_DOWN || !SW_RIGHT || !SW_LEFT ){
                counter = 1;
            }
            if(counter && SW_UP && SW_DOWN && SW_RIGHT && SW_LEFT){
                sequence = GAMEOVER;
                counter = 0;
                SystemTimer = 0;
            }
            if(SystemTimer > SECOND_100msec * 100){
                sequence = GAMEOVER;
                counter = 0;
                SystemTimer = 0;
            }
        }else if(sequence == FAIL){
            Miss();
            for(i=0;i<100;i++)   __delay_ms(6);
            sequence = GAMEOVER;
            SystemTimer = 0;
            for(i=0;i<8;i++){
                Stage_g[i]=0x00;
            }
            Stage_r[0]=0x00;
            Stage_r[1]=number[score_10][0];
            Stage_r[2]=number[score_10][1];
            Stage_r[3]=number[score_10][2];
            Stage_r[4]=0x00;
            Stage_r[5]=number[score_1][0];
            Stage_r[6]=number[score_1][1];
            Stage_r[7]=number[score_1][2];
            if(score_10 > high_10 ||((score_10 == high_10) && score_1 > high_1)){
                high_10 = score_10;
                high_1 = score_1;
                HighScore();
            }else{
                Score();
            }
        }else if(sequence == GAMEOVER){
            
            if(!SW_UP || !SW_DOWN || !SW_RIGHT || !SW_LEFT ){
                counter = 1;
            }
            if(counter && SW_UP && SW_DOWN && SW_RIGHT && SW_LEFT){
                sequence = POWERON;
                counter = 0;
                SystemTimer = 0;
            }
            if(SystemTimer >= SECOND_100msec * 200){
                sequence = POWERON;
                counter = 0;
                SystemTimer = 0;

            }
        }
    }
}
