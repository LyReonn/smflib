/*
 * waterfall.h
 *
 *  Created on: 2017-02-22
 *      Author: LyReonn
 */

#ifndef WATERFALL_H_
#define WATERFALL_H_

#define WATERFALL_SUCCESS			 0
#define WATERFALL_FILTER_FAILED		-1
#define WATERFALL_MULTIPLE_TEMPO	-2
#define WATERFALL_TOO_MANY_CHN		-3
#define WATERFALL_TOO_MANY_TRK		-4
#define WATERFALL_ONLY_ONE_CHN		-5
#define WATERFALL_CHN_INCORRECT		-6
#define WATERFALL_WRITE_FAILED		-7

int WriteWaterfall(const char *filename, SMFILE *smf);

#endif /* WATERFALL_H_ */
