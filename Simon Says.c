
#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

#include <ugpio/ugpio.h>

using namespace std;

enum playerStatus {ingame, loss, reset, win}; 
enum colour{red, blue, green, yellow, invalid, lost};
enum levelNum {ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN};

const unsigned int LL0=0; //level led 0 ---PIN 0
const unsigned int LL1=1; //level led 1 ---PIN 1
const unsigned int LL2=2; //level led 2 ---PIN 2
const unsigned int led0=3; //led 0 ---PIN 3 - RED PIN
const unsigned int led1=4; //led 1 ---PIN SCL - BLUE PIN
const unsigned int led2=5; //led 2 ---PIN SDA - GREEN PIN
const unsigned int led3=6; //led 3 ---PIN 6 - YELLOW PIN
const unsigned int button0=11; //button 0 ---PIN 11 - RED
const unsigned int button1=18; //button 1 ---PIN 18 - BLU
const unsigned int button2=19; //button 2 -- PIN 19 - GREEN
const unsigned int button3=45; //button 3 ---PIN TX1 - YELLOW
const unsigned int buzzer=46; //buzzer ---PIN RX1

const int maxNumberGames = 20; //Highest expected number of games that will be played (Used for the highest level array)
const int maxLevel = 8;
const int minLevel = 0;

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);

	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

	return buf;
}

class Statistics{
private:
	int levelReached; //The highest level reached for a particular run
	int highestOverallLevel[4]; //The highest level reached since powering on the machine
	int lowestOverallLevel[4]; //The lowest level reached since powering on the machine
	int levelReachedArray[maxNumberGames][4]; //Array holding the highest level reached for each run
	int modeArray[maxNumberGames][4];
	int numModes[4];
	int range[4];
	float averageLevel[4]; //The average level reached throuhgout all runs

public:
	//Constructor
	Statistics() {
		//Initialize all variables/arrays
		levelReached = 0;
		for (int counter = 0; counter < 4; counter++) {
			highestOverallLevel[counter] = 0;
			lowestOverallLevel[counter] = 0;
			numModes[counter] = 0;
			range[counter] = 0;
			averageLevel[counter] = 0;
		}
		for (int i = 0; i < maxNumberGames; i++) {
			for (int j = 0; j < 4; j++) {
				levelReachedArray[i][j] = 0;
				modeArray[i][j] = 0;
			}
		}
	}

	//Method that stores the level reached in the array
	int storeLevelReached(int newHighestlevel, int groupNumber) {
		int groupNum = groupNumber-1;
		levelReached = newHighestlevel;
		int counter = 0;
		int current = levelReachedArray[0][groupNum];
		//The array inititially is all 0
		while (current != 0) {
			counter++;
			current = levelReachedArray[counter][groupNum];
		}
		levelReachedArray[counter][groupNum] = newHighestlevel;
	}

	//Calculate the average level reached from all runs
	float calcAverageLevel(int numRuns[], int groupNumber) {
		int groupNum = groupNumber-1;
		int counter = 0;
		averageLevel[groupNum] = 0;
		while (counter < numRuns[groupNum]) {
			averageLevel[groupNum] += float(levelReachedArray[counter][groupNum]);
			counter++;
		}
		averageLevel[groupNum] = averageLevel[groupNum]/float(numRuns[groupNum]);
		return averageLevel[groupNum];
	}

	//Check what the highest overall score is
	int calcOverallHighest(int numRuns[], int groupNumber) {
		int groupNum = groupNumber-1;
		int current = minLevel;
		for (int counter = 0; counter < numRuns[groupNum]; counter++) {
			if (levelReachedArray[counter][groupNum] > current) {
				current = levelReachedArray[counter][groupNum];
			}
		}
		highestOverallLevel[groupNum] = current;
		return highestOverallLevel[groupNum];
	}

	//Check what the lowest overall score is
	int calcOverallLowest(int numRuns[], int groupNumber) {
		int groupNum = groupNumber-1;
		int current = maxLevel;
		bool empty = true;
		for (int counter = 0; counter < maxNumberGames; counter++) {
			if (levelReachedArray[counter][groupNum] != 0) {
				empty = false;
			}
		}
		if (empty == false) {
			for (int counter = 0; counter < numRuns[groupNum]; counter++) {
				if ((levelReachedArray[counter][groupNum] < current) && (levelReachedArray[counter][groupNum] != 0)){
					current = levelReachedArray[counter][groupNum];
				}
			}
			lowestOverallLevel[groupNum] = current;
		}
		return lowestOverallLevel[groupNum];
	}

	//Calculate the range of the dataset
	int calcRange(int numRuns[], int groupNumber) {
		int groupNum = groupNumber-1;
		if (numRuns[groupNum] <= 0) {
			return -1;
		}
		int counter;
		int currentNum;
		int min = maxLevel;
		int max = minLevel;
		for (counter = 0; counter < numRuns[groupNum]; counter++) {
			currentNum = levelReachedArray[counter][groupNum];
			if (currentNum < min) {
				min = currentNum;
			}
			if (currentNum > max) {
				max = currentNum;
			}
		}

		range[groupNum] = (max - min);
		return range[groupNum];
	}

	//Calculate the modes & the number of modes in the highest level array
	int calcModes(int numRuns[], int groupNumber) {
		int groupNum = groupNumber-1;
		bool marker = sort(numRuns[groupNum], groupNum);
		int counter;
		int counter2 = 0;
		int currentNum;
		int max = 1;
		int modeCounter[numRuns[groupNum]][2];
		for (int i = 0; i < numRuns[groupNum]; i++) {
			modeCounter[numRuns[groupNum]][0] = 0;
			modeCounter[numRuns[groupNum]][1] = 0;
		}
		if (marker == true) {
			//Increment through the sorted dataset
			for (counter = 0; counter < numRuns[groupNum]; counter++) {
				currentNum = levelReachedArray[counter][groupNum];
				if (counter == 0) {
					modeCounter[counter2][0] = currentNum;
					modeCounter[counter2][1] = 1;
					counter2++;
				}
				else {
					//If the current number is already accounted for in the mode array, increment the amount of that number
					if (currentNum == levelReachedArray[counter - 1][groupNum]) {
						modeCounter[counter2 - 1][1] = modeCounter[counter2 - 1][1] + 1;
					}
					else { //If not, create a new "entry" for that value in the mode array
						modeCounter[counter2][0] = currentNum;
						modeCounter[counter2][1] = 1;
						counter2++;
					}
				}            
			}
			int newCounter = 0;
			for (counter = 0; counter < counter2; counter++) {
				currentNum = modeCounter[counter][1];
				if (currentNum > max) {
					numModes[groupNum] = 1;
					newCounter = 0;
					max = currentNum;
					modeArray[0][groupNum] = modeCounter[counter][0];
					newCounter++;
				}
				else if (currentNum == max) {
					modeArray[newCounter][groupNum] = modeCounter[counter][0];
					newCounter++;
					numModes[groupNum]++;
				}
			}
		}
		else {
			return -1;
		}
		return numModes[groupNum];
	}

	//Selection Sort for the highest level array
	bool selection(int numRuns, int counter, int groupNum) {
		if (numRuns <= 0) {
			return false;
		}
		if (counter >= numRuns -1) {
			return true;
		}
		int counter2 = counter;
		for (int counter3 = counter + 1; counter3 < numRuns; counter3++) {
			if (levelReachedArray[counter3][groupNum] < levelReachedArray[counter2][groupNum]) {
				counter2 = counter3;
			}
		}
		int temp = levelReachedArray[counter][groupNum];
		levelReachedArray[counter][groupNum] = levelReachedArray[counter2][groupNum];
		levelReachedArray[counter2][groupNum] = temp;
		selection(numRuns, counter + 1, groupNum);
	}

	//Sort the highest level array
	bool sort(int numRuns, int groupNum) {
		int counter = 0;
		bool marker = selection(numRuns, counter, groupNum);
		return marker;
	}

	//Get functions for the member variables
	int getLevelReached() {
		return levelReached;
	}
	int getHighestOverallLevel(int groupNumber) {
		int groupNum = groupNumber-1;
		return highestOverallLevel[groupNum];
	}
	int getLowestOverallLevel(int groupNumber) {
		int groupNum = groupNumber-1;
		return lowestOverallLevel[groupNum];
	}
	int getAverageLevel(int groupNumber) {
		int groupNum = groupNumber-1;
		return averageLevel[groupNum];
	}
	int getNumModes(int groupNumber) {
		int groupNum = groupNumber-1;
		return numModes[groupNum];
	}
	int getMode(int number, int groupNumber) {
		int groupNum = groupNumber-1;
		return modeArray[number][groupNum];
	}

};

void logger(Statistics* stats, int numRuns[], int functionID, playerStatus playerWinning, int groupNum) {
	//Char array for log file name
	char logFilename[8] = {0};

	logFilename[0] = 108; //l
	logFilename[1] = 111; //o
	logFilename[2] = 103; //g
	logFilename[3] = 46; //Dot
	logFilename[4] = 116; //t
	logFilename[5] = 120; //x
	logFilename[6] = 116; //t
	logFilename[7] = 0; //null

	ofstream outfile;
	outfile.open (logFilename, ios::out | ios::app);

	if (functionID == 0) {
		outfile << "Timestamp: " << currentDateTime() << endl;
		outfile << "Function ID: " << functionID << " || Starting game, about to start LED pattern" << endl;
	}
	else if (functionID == 1) {
		outfile << "Timestamp: " << currentDateTime() << endl;
		outfile << "Group Number: " << groupNum << endl;
		outfile << "Function ID: " << functionID << " || Just finished choosing group number" << endl;
	}
	else if (functionID == 2) {
		outfile << "Timestamp: " << currentDateTime() << endl;
		outfile << "Function ID: " << functionID << " || Now displaying level LEDs" << endl;
	}
	else if (functionID == 3) {
		outfile << "Timestamp: " << currentDateTime() << endl;
		outfile << "Function ID: " << functionID << " || Now generating a LED pattern" << endl;
	}
	else if (functionID == 4) {
		outfile << "Timestamp: " << currentDateTime() << endl;
		outfile << "Function ID: " << functionID << " || Now receiving player's input..." << endl;
	}
	else if (functionID == 5) {
		if (playerWinning == win) {
			outfile << "Timestamp: " << currentDateTime() << endl;
			outfile << "Function ID: " << functionID << " || You won!" << endl;
		}
		else {
			outfile << "Timestamp: " << currentDateTime() << endl;
			outfile << "Function ID: " << functionID << " || You lost!" << endl;
		}
	}
	else if (functionID == 6) {
		outfile << "Timestamp: " << currentDateTime() << endl;
		outfile << "Function ID: " << functionID << " || Now iterating the pattern..." << endl;
	}
	else if (functionID == 7) {
		if (playerWinning == reset) {
			outfile << "Timestamp: " << currentDateTime() << endl;
			outfile << "Function ID: " << functionID << " || Now resetting!" << endl;
		}
		else {
			outfile << "Timestamp: " << currentDateTime() << endl;
			outfile << "Function ID: " << functionID << " || Now ending the game" << endl;
		}	
	}
	//If the player is done, print out all the statistics
	if (functionID == 7) {
		if (numRuns[0] != 0) {
			outfile << "-----------Group 1 Statistics-----------" << endl;
			outfile << "Highest level reached (since powering on): " << stats->calcOverallHighest(numRuns, 1) << endl;
			outfile << "Lowest level reached (since powering on): " << stats->calcOverallLowest(numRuns, 1) << endl;
			outfile << "Average level reached (since powering on): " << stats->calcAverageLevel(numRuns, 1) << endl;
			outfile << "Range of Levels reached: " << stats->calcRange(numRuns, 1) << endl;
			outfile << "Number of Modes: " << stats->calcModes(numRuns, 1) << endl;
			outfile << "	Modes: ";
			for (int i = 0; i < stats->getNumModes(1); i++) {
				outfile << stats->getMode(i, 1);
				if (i < (stats->getNumModes(1)-1)) {
					outfile << ", ";
				}
			}
			outfile << "\n" << endl;
		}
		if (numRuns[1] != 0) {
			outfile << "-----------Group 2 Statistics-----------" << endl;
			outfile << "Highest level reached (since powering on): " << stats->calcOverallHighest(numRuns, 2) << endl;
			outfile << "Lowest level reached (since powering on): " << stats->calcOverallLowest(numRuns, 2) << endl;
			outfile << "Average level reached (since powering on): " << stats->calcAverageLevel(numRuns, 2) << endl;
			outfile << "Range of Levels reached: " << stats->calcRange(numRuns, 2) << endl;
			outfile << "Number of Modes: " << stats->calcModes(numRuns, 2) << endl;
			outfile << "	Modes: ";
			for (int i = 0; i < stats->getNumModes(2); i++) {
				outfile << stats->getMode(i, 2);
				if (i < (stats->getNumModes(2)-1)) {
					outfile << ", ";
				}
			}
			outfile << "\n" << endl;
		}
		if (numRuns[2] != 0) {
			outfile << "-----------Group 3 Statistics-----------" << endl;
			outfile << "Highest level reached (since powering on): " << stats->calcOverallHighest(numRuns, 3) << endl;
			outfile << "Lowest level reached (since powering on): " << stats->calcOverallLowest(numRuns, 3) << endl;
			outfile << "Average level reached (since powering on): " << stats->calcAverageLevel(numRuns, 3) << endl;
			outfile << "Range of Levels reached: " << stats->calcRange(numRuns, 3) << endl;
			outfile << "Number of Modes: " << stats->calcModes(numRuns, 3) << endl;
			outfile << "	Modes: ";
			for (int i = 0; i < stats->getNumModes(3); i++) {
				outfile << stats->getMode(i, 3);
				if (i < (stats->getNumModes(3)-1)) {
					outfile << ", ";
				}
			}
			outfile << "\n" << endl;
		}
		if (numRuns[3] != 0) {
			outfile << "-----------Group 4 Statistics-----------" << endl;
			outfile << "Highest level reached (since powering on): " << stats->calcOverallHighest(numRuns, 4) << endl;
			outfile << "Lowest level reached (since powering on): " << stats->calcOverallLowest(numRuns, 4) << endl;
			outfile << "Average level reached (since powering on): " << stats->calcAverageLevel(numRuns, 4) << endl;
			outfile << "Range of Levels reached: " << stats->calcRange(numRuns, 4) << endl;
			outfile << "Number of Modes: " << stats->calcModes(numRuns, 4) << endl;
			outfile << "	Modes: ";
			for (int i = 0; i < stats->getNumModes(4); i++) {
				outfile << stats->getMode(i, 4);
				if (i < (stats->getNumModes(4)-1)) {
					outfile << ", ";
				}
			}
			outfile << "\n" << endl;
		}	
	}
	outfile.close();

}

void gpioInit ()
{
  gpio_request(0,NULL); //0
  gpio_direction_output(0,0);
  gpio_request(1,NULL); //1
  gpio_direction_output(1,0);
  gpio_request(2,NULL); //2
  gpio_direction_output(2,0);
  gpio_request(3,NULL); //3
  gpio_direction_output(3,0);
  gpio_request(4,NULL); //SCL
  gpio_direction_output(4,0);
  gpio_request(5,NULL); //SDA
  gpio_direction_output(5,0);
  gpio_request(6,NULL); //CS1
  gpio_direction_output(6,0);
  gpio_request(11,NULL); //11
  gpio_direction_input(11);
  gpio_request(18,NULL); //18
  gpio_direction_input(18);
  gpio_request(19,NULL); //19
  gpio_direction_input(19);
  gpio_request(45,NULL); //TX1
  gpio_direction_input(45);
  gpio_request(46,NULL); //RX1
  gpio_direction_output(46,0);
}

void gpioFree ()
{
  gpio_free(0);
  gpio_free(1);
  gpio_free(2);
  gpio_free(3);
  gpio_free(4);
  gpio_free(5);
  gpio_free(6);
  gpio_free(11);
  gpio_free(18);
  gpio_free(19);
  gpio_free(45);
  gpio_free(46);
}

playerStatus resetCheck()
{
	if (!gpio_get_value(button1) && !gpio_get_value(button3)){
		printf("GAME RESETED\n");
		while(!gpio_get_value(button1) && !gpio_get_value(button3)) //wait for release
			{};
		gpio_set_value(led0 , 1);
		usleep(100000);
		gpio_set_value(led0 , 0);
		gpio_set_value(led1 , 1);
		usleep(100000);
		gpio_set_value(led1 , 0);
		gpio_set_value(led2 , 1);
		usleep(100000);
		gpio_set_value(led2 , 0);
		gpio_set_value(led3 , 1);
		usleep(100000);
		gpio_set_value(led3 , 0);
		return reset;
	}
    else
      return ingame;
}


int GpoupChoice(int& group){
   while((gpio_get_value(button0))&&(gpio_get_value(button1))&&(gpio_get_value(button2))&&(gpio_get_value(button3)))
   {
	gpio_set_value(led0 , 0);
	gpio_set_value(led1 , 0);
	gpio_set_value(led2 , 0);
	gpio_set_value(led3 , 0);
	usleep(100000);
	gpio_set_value(led0 , 1);
	gpio_set_value(led1 , 1);
	gpio_set_value(led2 , 1);
	gpio_set_value(led3 , 1);
	usleep(100000);   
   }; //waiting for choise
   
   	gpio_set_value(led0 , 1);
	gpio_set_value(led1 , 1);
	gpio_set_value(led2 , 1);
	gpio_set_value(led3 , 1);
	
	if (!gpio_get_value(button0))
		group=1;
	else if(!gpio_get_value(button1))
		group=2;
	else if(!gpio_get_value(button2))
		group=3;
	else
		group=4;
	
}

playerStatus keepPlaying(playerStatus decision)
{
    bool decisionMade = false;
    printf("Would you like to keep playing?\n");
	while (!decisionMade)
    {
       gpio_set_value(4, 1);
       gpio_set_value(6, 1);
       usleep(100000);
       gpio_set_value(4, 0);
       gpio_set_value(6, 0);
       usleep(100000);
       if (!gpio_get_value(button1))
       {
	printf("You decided to reset! \n");
          decisionMade = true;
          decision = reset;
       }
       else if (!gpio_get_value(button3))
       {
	printf("You quit \n");
          decisionMade = true;
       }
   }
   return decision;
}

int BuzzerOn(int part){
	
	switch (part){
	
		case 0:
			for(int i=0;i<500;i++){ // 555 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(1000); 
				
				gpio_set_value(buzzer , 0);
				usleep(1000); 
			}
		break;
		
		case 1:
			for(int i=0;i<150;i++){ // 625 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(800); 
				
				gpio_set_value(buzzer , 0);
				usleep(800); 
			}
		break;
		
		case 2:
			for(int i=0;i<500;i++){ // 833 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(600); 
				
				gpio_set_value(buzzer , 0);
				usleep(600); 
			}
		break;
		
		case 3:
			for(int i=0;i<400;i++){ // 1250 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(400); 
				
				gpio_set_value(buzzer , 0);
				usleep(400); 
			}
		break;
		
		case 4:
			for(int i=0;i<1666;i++){ // 1666 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(300); 
				
				gpio_set_value(buzzer , 0);
				usleep(300); 
			}
		break;

		default:
			return -1;
		break;
	}
				
	return 0;
}



int BuzzerLose(int part){
	
	switch (part){
	
		case 0:
			for(int i=0;i<1670;i++){ // 2500 is 1 sec with this frequency
					gpio_set_value(buzzer , 1);
					usleep(300); 
					
					gpio_set_value(buzzer , 0);
					usleep(300); 
				}
		break;
		
		case 1:
			for(int i=0;i<50;i++){ // 1250 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(400); 
				
				gpio_set_value(buzzer , 0);
				usleep(400); 
			}
		break;
		
		case 2:
			for(int i=0;i<500;i++){ // 833 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(600); 
				
				gpio_set_value(buzzer , 0);
				usleep(600); 
			}
		break;
		
		case 3:
			for(int i=0;i<400;i++){ // 625 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(800); 
				
				gpio_set_value(buzzer , 0);
				usleep(800); 
			}
		break;
		
		case 4:
			for(int i=0;i<600;i++){ // 500 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(1000); 
				
				gpio_set_value(buzzer , 0);
				usleep(1000); 
			}
		break;

		default:
			return -1;
		break;
	}
				
	return 0;
}




int BuzzerWin(int part){
	
	switch (part){
	
		case 0:
			for(int i=0;i<100;i++){ // 500 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(1000); 
				
				gpio_set_value(buzzer , 0);
				usleep(1000); 
			}
		break;
		
		case 1:
			for(int i=0;i<125;i++){ // 625 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(800); 
				
				gpio_set_value(buzzer , 0);
				usleep(800); 
			}
		break;
		
		case 2:
			for(int i=0;i<166;i++){ // 833 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(600); 
				
				gpio_set_value(buzzer , 0);
				usleep(600); 
			}
		break;
		
		case 3:
			for(int i=0;i<250;i++){ // 1250 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(400); 
				
				gpio_set_value(buzzer , 0);
				usleep(400); 
			}
		break;
		
		case 4:
			for(int i=0;i<2000;i++){ // 1666 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(300); 
				
				gpio_set_value(buzzer , 0);
				usleep(300); 
			}		
		break;

		default:
			return -1;
		break;
	}
				
	return 0;
}



int BuzzerBeep(int button){
	
	switch (button){
		
		case 0:
			for(int i=0;i<250;i++){ // 2500 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(300); 
				
				gpio_set_value(buzzer , 0);
				usleep(300); 
			}		
		break;
		
		case 1:
			for(int i=0;i<55;i++){ // 555 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(900); 
				
				gpio_set_value(buzzer , 0);
				usleep(900); 
			}	
		break;
		
		case 2:
			for(int i=0;i<62;i++){ // 625 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(800); 
				
				gpio_set_value(buzzer , 0);
				usleep(800); 
			}	
		break;
		
		case 3:
			for(int i=0;i<83;i++){ // 833 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(600); 
				
				gpio_set_value(buzzer , 0);
				usleep(600); 
			}	
		break;
		
		case 4:
			for(int i=0;i<125;i++){ // 1250 is 1 sec with this frequency
				gpio_set_value(buzzer , 1);
				usleep(400); 
				
				gpio_set_value(buzzer , 0);
				usleep(400); 
			}	
		break;
		
		default:
		return false;
		break;		
	}
	return 0;	
}



void LLStart(colour passedColour, bool allOff) 
{
  if (allOff == true) 
  {
    gpio_set_value(0, 0);
    gpio_set_value(1, 0);
    gpio_set_value(2, 0);
  }

  else 
  {
    switch (passedColour) 
    {
      case red:
      gpio_set_value(0, 1);
      gpio_set_value(1, 0);
      gpio_set_value(2, 0);
        break;

        case blue:
      gpio_set_value(0, 0);
      gpio_set_value(1, 1);
      gpio_set_value(2, 0);
        break;

        case green:
      gpio_set_value(0, 0);
      gpio_set_value(1, 0);
      gpio_set_value(2, 1);
        break;

        case yellow:
      gpio_set_value(0, 0);
      gpio_set_value(1, 0);
      gpio_set_value(2, 0);
        break;
    }
  }
}

void turnOnLight (colour passedColour,const float microSecondsOn) // turns on the light of the passed colour
{
  float x = microSecondsOn;
  switch (passedColour)
  {
    case red:
    gpio_set_value(3, 1);
    BuzzerBeep(1);
    usleep(x); // x = seconds to be decided
    gpio_set_value(3, 0);
    break;

    case blue:
    //GPIO PIN
    gpio_set_value(4, 1);
    BuzzerBeep(2);
    usleep(x); // x = seconds to be decided
    gpio_set_value(4, 0);
    break;

    case green:
    //GPIO PIN
    gpio_set_value(5, 1);
    BuzzerBeep(3);
    usleep(x); // x = seconds to be decided
    gpio_set_value(5, 0);
    break;

    case yellow:
    //GPIO PIN
    gpio_set_value(6, 1);
    BuzzerBeep(4);
    usleep(x); // x = seconds to be decided
    gpio_set_value(6, 0);
    break;
  }
}

colour ledRandomizer ()
{
    srand(time(NULL));
    int ledRandNum = rand() % 4;
    switch (ledRandNum)
    {
	case 0:
	return red;
	break;

	case 1:
	return blue;
	break;

	case 2:
	return green;
	break;

	case 3:
	return yellow;
	break;
    }
}

void turnOnOnce()
{
    gpio_set_value(3, 1);
    gpio_set_value(4, 1);
    gpio_set_value(5, 1);
    gpio_set_value(6, 1);
    usleep(100000);
}

void turnOffOnce()
{
    gpio_set_value(3, 0);
    gpio_set_value(4, 0);
    gpio_set_value(5, 0);
    gpio_set_value(6, 0);
    usleep(100000);
}

void startLed()
{
	gpio_set_value(led0 , 1);
	gpio_set_value(LL0 , 1);
	BuzzerOn(0);
	gpio_set_value(led1 , 1);
	gpio_set_value(LL1 , 1);
	BuzzerOn(1);
	gpio_set_value(led2 , 1);
	gpio_set_value(LL2 , 0);
	BuzzerOn(2);
	gpio_set_value(led3 , 1);
	gpio_set_value(LL0 , 0);
	BuzzerOn(3);
	gpio_set_value(LL1 , 0);
	BuzzerOn(4);
	
	//blink
	gpio_set_value(led0 , 0);
	gpio_set_value(led1 , 0);
	gpio_set_value(led2 , 0);
	gpio_set_value(led3 , 0);
	gpio_set_value(LL0 , 0);
	gpio_set_value(LL1 , 0);
	gpio_set_value(LL2 , 0);
	usleep(100000);
	gpio_set_value(led0 , 1);
	gpio_set_value(led1 , 1);
	gpio_set_value(led2 , 1);
	gpio_set_value(led3 , 1);
	gpio_set_value(LL0 , 1);
	gpio_set_value(LL1 , 1);
	gpio_set_value(LL2 , 1);
	usleep(100000);
	gpio_set_value(led0 , 0);
	gpio_set_value(led1 , 0);
	gpio_set_value(led2 , 0);
	gpio_set_value(led3 , 0);
	gpio_set_value(LL0 , 0);
	gpio_set_value(LL1 , 0);
	gpio_set_value(LL2 , 0);
	
}

void loseLed()
{
	gpio_set_value(led0 , 1);
	gpio_set_value(led1 , 1);
	gpio_set_value(led2 , 1);
	gpio_set_value(led3 , 1);
	gpio_set_value(LL0 , 1);
	gpio_set_value(LL1 , 1);
	gpio_set_value(LL2 , 1);
	usleep(500000);
	
	gpio_set_value(led0 , 0);
	BuzzerLose(0);
	gpio_set_value(led1 , 0);
	BuzzerLose(1);
	gpio_set_value(led2 , 0);
	BuzzerLose(2);
	gpio_set_value(led3 , 0);
	BuzzerLose(3);
	gpio_set_value(LL0 , 0);
	gpio_set_value(LL1 , 0);
	gpio_set_value(LL2 , 0);
	BuzzerLose(4);
	
	//blink
	usleep(500000);
	gpio_set_value(led0 , 1);
	gpio_set_value(led1 , 1);
	gpio_set_value(led2 , 1);
	gpio_set_value(led3 , 1);
	gpio_set_value(LL0 , 1);
	gpio_set_value(LL1 , 1);
	gpio_set_value(LL2 , 1);
	usleep(100000);
	gpio_set_value(led0 , 0);
	gpio_set_value(led1 , 0);
	gpio_set_value(led2 , 0);
	gpio_set_value(led3 , 0);
	gpio_set_value(LL0 , 0);
	gpio_set_value(LL1 , 0);
	gpio_set_value(LL2 , 0);

}

void winLed()
{
    gpio_set_value(led0 , 1);
	gpio_set_value(LL0 , 1);
	BuzzerWin(0);
	gpio_set_value(led1 , 1);
	BuzzerWin(1);
	gpio_set_value(led2 , 1);
	gpio_set_value(LL1 , 1);
	BuzzerWin(2);
	gpio_set_value(led3 , 1);
	BuzzerWin(3);
	gpio_set_value(LL2 , 1);
	BuzzerWin(4);
	
	//blink
	gpio_set_value(led0 , 0);
	gpio_set_value(led1 , 0);
	gpio_set_value(led2 , 0);
	gpio_set_value(led3 , 0);
	gpio_set_value(LL0 , 0);
	gpio_set_value(LL1 , 0);
	gpio_set_value(LL2 , 0);
	usleep(100000);
	gpio_set_value(led0 , 1);
	gpio_set_value(led1 , 1);
	gpio_set_value(led2 , 1);
	gpio_set_value(led3 , 1);
	gpio_set_value(LL0 , 1);
	gpio_set_value(LL1 , 1);
	gpio_set_value(LL2 , 1);
	usleep(100000);
	gpio_set_value(led0 , 0);
	gpio_set_value(led1 , 0);
	gpio_set_value(led2 , 0);
	gpio_set_value(led3 , 0);
	gpio_set_value(LL0 , 0);
	gpio_set_value(LL1 , 0);
	gpio_set_value(LL2 , 0);
}        
   

   
void patternEvolve (colour* arrayInitial, int patternIteration)
{
    arrayInitial[patternIteration + 1] = ledRandomizer();
    arrayInitial[patternIteration + 2] = ledRandomizer();
}




colour getUserInput (float& levelTime, float* pushTime, int pushTimeIndex) // requests ugpio
{
	float push=0;
  colour userInput = invalid;
  while (userInput == invalid)
  {
	push++; //counting time between pushes
	levelTime++; //level time
    if (!gpio_get_value(11))
    {
		pushTime[pushTimeIndex]=push*0.005; //push time
		push=0;
		
      while(!gpio_get_value(11))
      {
          	levelTime++; //level time
			push++; //counting time between pushes
			
			if(push>1000) //5 sec in 580Mhz clock -> if holds too long
				return lost;
		  turnOnLight(red, 5000);
      }
      return red;
    }/////////////////
	
    else if (!gpio_get_value(18))
    {
       pushTime[pushTimeIndex]=push*0.005; //push time
	   push=0;
	   while(!gpio_get_value(18))
       {
          	levelTime++; //level time
			push++; //counting time between pushes
			
			if(push>1000) //5 sec in 580Mhz clock
				return lost;
		  turnOnLight(blue, 5000);
       }
       return blue;
    }/////////////////
	
    else if (!gpio_get_value(19))
    { 
       pushTime[pushTimeIndex]=push*0.005; //push time
	   push=0;
	   while(!gpio_get_value(19))
       {
          	levelTime++; //level time
			push++; //counting time between pushes
			
			if(push>1000) //5 sec in 580Mhz clock
				return lost;
		  turnOnLight(green, 5000);
       }
       return green;
    }/////////////////
	
    else if (!gpio_get_value(45))
    {
       pushTime[pushTimeIndex]=push*0.005; //push time
	   push=0;
	   while(!gpio_get_value(45))
       {
          	levelTime++; //level time
			push++; //counting time between pushes
			
			if(push>1000) //5 sec in 580Mhz clock
				return lost;
		  turnOnLight(yellow, 5000);
       }
       return yellow;
    }/////////////////
	
    usleep(5000);//5mSec sleep
	
	if(push>1000) //5 sec in 580Mhz clock -> if holds too long
		return lost;
	
  }
}




playerStatus userPatternCheck (colour* pattern, int patternSize, float& levelTime,float* pushTime)
{
   int pushTimeIndex=0;
   colour userArray[15];
   int iterator = 0;
   while (iterator < patternSize)
   {
       userArray[iterator] = getUserInput(levelTime,pushTime,pushTimeIndex);
       pushTimeIndex++;
	   
	   if ((pattern[iterator] != userArray[iterator]) || (userArray[iterator]==lost))
       {
           return loss;
       }
       iterator ++;
   }
   return ingame;
}

void levelLEDs(int levelNumber) {

  levelNum currentLevel;

  if (levelNumber == 1) {
    currentLevel = ONE;
  }

  else if (levelNumber == 2) {
    currentLevel = TWO;
  }

  else if (levelNumber == 3) {
    currentLevel = THREE;
  }

  else if (levelNumber == 4) {
    currentLevel = FOUR;
  }

  else if (levelNumber == 5) {
    currentLevel = FIVE;
  }

  else if (levelNumber == 6) {
    currentLevel = SIX;
  }

  else if (levelNumber == 7) {
    currentLevel = SEVEN;
  }

    switch(currentLevel) 
    {
    case ONE:
      gpio_set_value(0, 1);
      gpio_set_value(1, 0);
      gpio_set_value(2, 0);
      break;

    case TWO: 
      gpio_set_value(0, 0);
      gpio_set_value(1, 1);
      gpio_set_value(2, 0);
      break;

    case THREE:
      gpio_set_value(0, 1);
      gpio_set_value(1, 1);
      gpio_set_value(2, 0);
      break;

    case FOUR:
      gpio_set_value(0, 0);
      gpio_set_value(1, 0);
      gpio_set_value(2, 1);
      break;

    case FIVE: 
      gpio_set_value(0, 1);
      gpio_set_value(1, 0);
      gpio_set_value(2, 1);
      break;

    case SIX: 
      gpio_set_value(0, 0);
      gpio_set_value(1, 1);
      gpio_set_value(2, 1);
      break;

    case SEVEN: 
      gpio_set_value(0, 1);
      gpio_set_value(1, 1);
      gpio_set_value(2, 1);
      break;
   }
}

void ledPattern (colour* pattern, int patternSize)
{
   int iterator = 0;
   while (iterator < patternSize)
   {
       turnOnLight (pattern[iterator], 1000000);
       usleep(200000);
       iterator++;
   }
}

int main()
{
	Statistics stats;
	int numRuns[4];
	for (int counter = 0; counter < 4; counter++) {
		numRuns[counter] = 0;
		}
   int group=0; //group choise between 1-4
   gpioInit(); // initializes pins
   colour currentPattern[15]; // pattern array
   int patternIteration; //pattern iteration
   int currentLevel; //current level used for level leds
   playerStatus playerWinning = reset; //player status initialized as reset (starting position)
   
   printf("Starting game... \n");
   logger(&stats, numRuns, 0, playerWinning, group);/////////////////
   startLed(); // startLed pattern function

   float levelTime=0; //level time
   float pushTime[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //time between presses
   
   currentPattern[patternIteration] = ledRandomizer(); // randomizes the first led   
   while (playerWinning == reset) // If the player decides to reset the game, then the game restarts from here
   {
      printf("Please choose a group between 1-4\n");
      GpoupChoice(group); //choose group
      logger(&stats, numRuns, 1, playerWinning, group);/////////////////
	
   	usleep(1000000); //1 sec delay
   	printf("You chose group %d\n",group);
   	usleep(1000000); //1 sec delay
   	printf("Ready? \n");
   	usleep(1000000); //1 sec delay
   	gpio_set_value(led0 , 0);
	gpio_set_value(led1 , 0);
	gpio_set_value(led2 , 0);
	gpio_set_value(led3 , 0);
	usleep(500000); //0.5 sec delay
 	//initalizes all game variables
       printf("start!\n");
	   playerWinning = ingame;
       patternIteration = 0;
       currentLevel = 0;
       while (patternIteration <= 14 && playerWinning == ingame) //ingame means player has not lost, won, or reset the game
       {
          if(patternIteration>0)
			printf("Level %d...\n",currentLevel);
		  levelLEDs(currentLevel); // function to display the correct level leds
		  logger(&stats, numRuns, 2, playerWinning, group);/////////////////
          ledPattern(currentPattern, patternIteration); // function to generate the pattern (generates it by iterations of +2
          logger(&stats, numRuns, 3, playerWinning, group);/////////////////
          playerWinning = resetCheck(); //checks for reset -> needs to be fixed
		  
	  if (playerWinning != reset) // != reset (user did not reset the game), the game continues to the user phase
	  {
		 if(patternIteration>0)
			 printf("receiving player's input \n");
			logger(&stats, numRuns, 4, playerWinning, group);/////////////////
		 levelTime=0; //reset
		  for(int i=0;i<14;i++) //reset
			  pushTime[i]=0;
				  
		 playerWinning = userPatternCheck(currentPattern, patternIteration,levelTime,pushTime); //player plays
		 
		 if(patternIteration>0){
			 levelTime = levelTime*0.005-0.005; //level time in seconds
			 printf("Time the level took: %.2f seconds\n",levelTime);//level time in seconds
			printf("Time between presses: ");//
			for(int i=0;pushTime[i]>0.001;i++)
				printf("%.2f ",pushTime[i]);//level time in seconds
			printf("\n");
			}
          }
	  currentLevel++; // iterates level
	  if (currentLevel == 8) // if win
	  {
	  	stats.storeLevelReached(currentLevel-1, group);
		playerWinning = win;
		logger(&stats, numRuns, 5, playerWinning, group);/////////////////
                printf("YOU WIN! :)\n");
				winLed();
  		playerWinning = keepPlaying(playerWinning);
  		numRuns[group-1]++;////////////////
	  }
          else if (playerWinning == loss)
          {
			  levelTime=0; //reset
			  stats.storeLevelReached(currentLevel-1, group);
				logger(&stats, numRuns, 5, playerWinning, group);/////////////////
			  for(int i=0;i<14;i++) //reset
				  pushTime[i]=0;
				 printf("YOU LOST! :(\n");
              loseLed();
		playerWinning = keepPlaying(playerWinning);    
		numRuns[group-1]++;////////////////      
}
	  else //continues the game -> iteration +2 for the next pattern
	  {
		 //printf("iteration occurs \n");
		 patternEvolve(currentPattern, patternIteration); //adds the new pattern
		 patternIteration += 2;
		 logger(&stats, numRuns, 6, playerWinning, group);/////////////////
	  }
	  usleep(1000000); //1 sec delay
       }
       logger(&stats, numRuns, 7, playerWinning, group);/////////////////
   }	
gpioFree();
}
