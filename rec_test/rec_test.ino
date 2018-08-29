#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

//write wav
unsigned long ChunkSize = 0L;
unsigned long Subchunk1Size = 16;
unsigned int AudioFormat = 1;
unsigned int numChannels = 1;
unsigned long sampleRate = 44100;
unsigned int bitsPerSample = 16;
unsigned long byteRate = sampleRate*numChannels*(bitsPerSample/8);// samplerate x channels x (bitspersample / 8)
unsigned int blockAlign = numChannels*bitsPerSample/8;
unsigned long Subchunk2Size = 0L;
unsigned long recByteSaved = 0L;
unsigned long NumSamples = 0L;
byte byte1, byte2, byte3, byte4;

const int myInput = AUDIO_INPUT_LINEIN;


// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=69,280.5
AudioInputI2S            i2s1;           //xy=119.3055419921875,219.4722137451172
AudioMixer4              mixer1;         //xy=211.49999321831598,294.24998643663196
AudioFilterStateVariable filter1;        //xy=338.999993218316,293.99998643663196
AudioEffectReverb        reverb1;        //xy=457.9999932183159,281.99998643663196
AudioRecordQueue         queue1;         //xy=487.61114501953125,136.80555725097656
AudioMixer4              mixer2;         //xy=623,240
AudioOutputI2S           i2s2;           //xy=752.3611450195312,235.66665649414062
AudioConnection          patchCord1(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord2(playSdWav1, 1, mixer1, 1);
AudioConnection          patchCord3(i2s1, 0, queue1, 0);
AudioConnection          patchCord4(i2s1, 1, mixer2, 0);
AudioConnection          patchCord5(mixer1, 0, filter1, 0);
AudioConnection          patchCord6(mixer1, 0, filter1, 1);
AudioConnection          patchCord7(filter1, 0, reverb1, 0);
AudioConnection          patchCord8(reverb1, 0, mixer2, 1);
AudioConnection          patchCord9(mixer2, 0, i2s2, 0);
AudioConnection          patchCord10(mixer2, 0, i2s2, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=379.77777099609375,429.5555419921875
// GUItool: end automatically generated code


int mode = 0; // 0=stopped, 1=recording, 2=playing
File frec;

// Use these with the Teensy 3.6 & Audio Shield
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used


// Use these with the Teensy Audio Shield
//#define SDCARD_CS_PIN    10
//#define SDCARD_MOSI_PIN  7
//#define SDCARD_SCK_PIN   14


void setup(){
  Serial.begin(115200);
  
  AudioMemory(60);
  
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.7);

    mixer1.gain(0,0.2);
    mixer1.gain(1,0.2);  

    filter1.frequency(1000);
    reverb1.reverbTime(20);

    mixer2.gain(0,0.5);
    mixer2.gain(1,0.5);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

}

void loop(){
  if(Serial.available() > 0) {
     int incomingInt = Serial.read();
     
     //REC
     if(incomingInt == 1){
        if(mode == 2) stopPlaying();
        if(mode == 0) startRecording();
     }
     //STOP
     if(incomingInt == 2){
        if(mode == 1) stopRecording();
        if(mode == 2) stopPlaying();        
     }
     //PLAY
     if(incomingInt == 3) {
      if(mode == 1) stopRecording();
      if(mode == 0) startPlaying();
     }
  }
  if (mode == 1) {
    continueRecording();
  }  
}

void startRecording() {

  if(SD.exists("RECORD.WAV")){
    SD.remove("RECORD.WAV");
  }
  frec = SD.open("RECORD.WAV",FILE_WRITE);
  if(frec) {
    queue1.begin();
    mode = 1;
    recByteSaved = 0L;
  }
}

void continueRecording() {
  if(queue1.available() >= 2){
    byte buffer[512];
    
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer + 256, queue1.readBuffer(),256);
    queue1.freeBuffer();
   
    frec.write(buffer, 512);
    recByteSaved += 512;
  }
}

void stopRecording(){
  queue1.end();
  if(mode == 1){
    while(queue1.available() > 0){
      
      frec.write((byte*)queue1.readBuffer(), 256);
      queue1.freeBuffer();
      recByteSaved += 256;
    }
    writeOutHeader();
    frec.close();

  }
  mode = 0;
}

void startPlaying(){  
  playSdWav1.play("RECORD.WAV");
  mode = 2;
}

void  stopPlaying(){
  if (mode == 2) playSdWav1.stop();
  mode = 0;
}

void writeOutHeader() { // update WAV header with final filesize/datasize

//  NumSamples = (recByteSaved*8)/bitsPerSample/numChannels;
//  Subchunk2Size = NumSamples*numChannels*bitsPerSample/8; // number of samples x number of channels x number of bytes per sample
  Subchunk2Size = recByteSaved;
  ChunkSize = Subchunk2Size + 36;
  frec.seek(0);
  frec.write("RIFF");
  byte1 = ChunkSize & 0xff;
  byte2 = (ChunkSize >> 8) & 0xff;
  byte3 = (ChunkSize >> 16) & 0xff;
  byte4 = (ChunkSize >> 24) & 0xff;  
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  frec.write("WAVE");
  frec.write("fmt ");
  byte1 = Subchunk1Size & 0xff;
  byte2 = (Subchunk1Size >> 8) & 0xff;
  byte3 = (Subchunk1Size >> 16) & 0xff;
  byte4 = (Subchunk1Size >> 24) & 0xff;  
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  byte1 = AudioFormat & 0xff;
  byte2 = (AudioFormat >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2); 
  byte1 = numChannels & 0xff;
  byte2 = (numChannels >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2); 
  byte1 = sampleRate & 0xff;
  byte2 = (sampleRate >> 8) & 0xff;
  byte3 = (sampleRate >> 16) & 0xff;
  byte4 = (sampleRate >> 24) & 0xff;  
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  byte1 = byteRate & 0xff;
  byte2 = (byteRate >> 8) & 0xff;
  byte3 = (byteRate >> 16) & 0xff;
  byte4 = (byteRate >> 24) & 0xff;  
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  byte1 = blockAlign & 0xff;
  byte2 = (blockAlign >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2); 
  byte1 = bitsPerSample & 0xff;
  byte2 = (bitsPerSample >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2); 
  frec.write("data");
  byte1 = Subchunk2Size & 0xff;
  byte2 = (Subchunk2Size >> 8) & 0xff;
  byte3 = (Subchunk2Size >> 16) & 0xff;
  byte4 = (Subchunk2Size >> 24) & 0xff;  
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  frec.close();

}


