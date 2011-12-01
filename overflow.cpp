            
            //Starting off the Peak Detector Sequence
            //Slope Bool holds the current state of the slope on the wave
            //PeakBuf holds the indices of the peaks
            //PeakTicker is the counter for PeakBuf
            
            
            fprintf(log, "Peak Detection Starting");
            slope = cal_slope(adcInputBuffer[2], adcInputBuffer[1]);
            fprintf(log, "Slope starts %s", slope);
            int PeakBuf[10];
                memset(PeakBuf, 0, sizeof(PeakBuf));
            int diffBuff[9];
				memset(diffBuff, 0, sizeof(PeakBuf));
			int PeakTicker = 0;
            double sum =0;
            for(int i = 1; i < SAMPLE_BUFFER_LENGTH - 1; i++){
                fprintf(log, "Slope is %s @ index %d\n", slope,i);
                switch(cal_slope(adcInputBuffer[i], adcInputBuffer[i-1])){
					case 1:
						if (slope == false){  // This is the condition in which you have found a Peak
							PeakBuf[PeakTicker] = i - 1; //Detects Peak After it has passed
							PeakTicker++;				 //Increment index of PeakBuf
							fprintf(log, "There is a peak @ index %d\n",i);

						 }
						 break;
					default:
						break;
				}
				if (PeakTicker == 10) break;				//Once the Peak Buffer is full quit
			}
			for (int i = 1; i ; i++){
				diffBuff[i] = PeakBuf[i] + PeakBuf[i+1];	 
			}
			for (int i = 0; i < sizeof(PeakBuf); i++){
				sum += diffBuff[i];
			}
			int mean = 200000/sum;
			
			fprintf(log,"The estimated frequency is %d\n", (int)mean);
			fclose(log);
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
	bool cal_slope(int x, int y) {
	bool res = false;;
	if (x-y > 0) res = true; 
	return res;
}	
