// this approach uses my library. Side effect is that it halts button input processing. Dunno why

// 		a2dData[0] = mcp3304_get13(0x08,0x00);
// 		a2dData[1] = mcp3304_get13(0x09,0x00);
//
// 		a2d_recent = mcp3304_get12(0x0f,0x80,1);
//
// 		if (((a2dData[2]-a2d_recent)<-4) || ((a2dData[2]-a2d_recent)>4))
// 		{
// 			if ((a2dData[2]-a2d_recent)<0)
// 			{
// 				a2dData[2] = a2d_recent+1;
// 			}
// 			else
// 			{
// 				if (a2dData[2]>1)
// 				{
// 					a2dData[2] = a2d_recent-1;
// 				}
// 			}
// 		}
// 		else
// 		{
// 			a2dData[2] = a2dData[2];
// 		}

//a2dData[2] = mcp3304_get12(0x0f,0x80,1);

		//////////////////////////////////////////////////////////////////////////
		//			DATA MODIFY
		//////////////////////////////////////////////////////////////////////////
		else
		{
			cycleCounter=0;
			for (i=0; i<3; i++) {a2dData[i] = a2dData[i]/128;}

			//////////////////////////////////////////////////////////////////////////

			// 			if (bit_is_set(device_configuration, 0))				// gather correction data
			// 			{
			// 				for (uchar i=0; i<2; i++)
			// 				{
			// 					if (a2dData[i]<a2dDataMinimums[i])
			// 					{
			// 						a2dDataMinimums[i]=a2dData[i];
			// 					}
			// 					if (a2dDataMaximums[i]<a2dData[i])
			// 					{
			// 						a2dDataMaximums[i]=a2dData[i];
			// 					}
			// 				}
			// 			}
			
			
			for (i=0; i<2; i++)
			{
				if (a2dDataMinimums[i]>a2dData[i])
				{
					a2dDataMinimums[i]=a2dData[i];
				}
				if (a2dDataMaximums[i]<a2dData[i])
				{
					a2dDataMaximums[i]=a2dData[i];
				}
			}
			//////////////////////////////////////////////////////////////////////////
			
			if (bit_is_set(device_configuration, 1))				// set center point and store correction data to eeprom
			{
				for (i=0; i<2; i++)
				{
					a2dDataCenters[i] = a2dData[i];
					device_configuration &= ~(1<<1);
				}
				//correction_data_store();
			}

			//////////////////////////////////////////////////////////////////////////
			
			// 			if (bit_is_set(device_configuration, 7))				// add trim information
			// 			{
			// 				for (uchar i=0; i<2; i++)
			// 				{
			// 					if (a2dData[i+3]<-a2dData[i]+0x0800)
			// 					{
			// 						a2dData[i+3] = -a2dData[i]+0x0800;
			// 					}
			// 					if (a2dData[i+3]>-a2dData[i]+0x0fff+0x0800)
			// 					{
			// 						a2dData[i+3] = -a2dData[i]+0x0fff+0x0800;
			// 					}
			//
			// 					a2dData[i] += (a2dData[i+3] - 0x0800);
			// 				}
			// 			}
			
			//////////////////////////////////////////////////////////////////////////
			
			// map data with our correction data
			//	map ( x, in_min, in_max, out_min , out_max )
			for (i=0; i<2; i++)
			{
				// 				if (a2dData[i]<a2dDataMinimums[i])
				// 				{
				// 					a2dData[i] = a2dDataMinimums[i];
				// 				}
				//
				// 				if (a2dData[i]>a2dDataMaximums[i])
				// 				{
				// 					a2dData[i] = a2dDataMaximums[i];
				// 				}
				
				//a2dData[i] = map(a2dData[i], a2dDataMinimums[i], a2dDataMaximums[i], 0, 8191);
				
				// 				if (a2dData[i]<a2dDataCenters[i])
				// 				{
				// 					a2dData[i] = map(a2dData[i], a2dDataMinimums[i], a2dDataCenters[i], 0, 4096);
				// 				}
				// 				else
				// 				{
				// 					a2dData[i] = map(a2dData[i], a2dDataCenters[i], a2dDataMaximums[i], 4096, 8192);
				// 				}
				
				
				
				// 				if (a2dData[i]<a2dDataCenters[i])
				// 				{
				// 					a2dData[i] = map(a2dData[i], 0, 4096, 0, 4096);
				// 				}
				// 				else
				// 				{
				// 					a2dData[i] = map(a2dData[i], 4096, 8192, 4096, 8192);
				// 				}
			}
			
//////////////////////////////////////////////////////////////////////////

this version with typecasting works
			
			for (i=0; i<2; i++)
			{
				if ((uint16_t)a2dDataMaximums[i]<(uint16_t)a2dData[i])
				{
					a2dDataMaximums[i]=(uint16_t)a2dData[i];
				}
				
				if ((long long)a2dData[i]<(long long)a2dDataMinimums[i])
				{
					a2dDataMinimums[i] = (uint16_t)a2dData[i];
				}
			}

this also works			
			for (i=0; i<2; i++)
			{
				if ((uint16_t)a2dDataMaximums[i]<(uint16_t)a2dData[i])
				{
					a2dDataMaximums[i]=(uint16_t)a2dData[i];
				}
								
				if ((int16_t)a2dData[i]<(int16_t)a2dDataMinimums[i])
				{
					a2dDataMinimums[i] = (uint16_t)a2dData[i];
				}
			}