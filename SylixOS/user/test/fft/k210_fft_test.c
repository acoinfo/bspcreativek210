/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: k210_fft_test.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 25 ��
**
** ��        ��: K210 ������ FFT �������Գ���
*********************************************************************************************************/
#include "pthread.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

#include "driver/fft/k210_fft.h"
/*********************************************************************************************************
 �궨��
*********************************************************************************************************/
#define PI          3.14158265389793
#define FFT_N       512
/*********************************************************************************************************
 ȫ�ֱ�������
*********************************************************************************************************/
INT16   real[FFT_N];
INT16   imag[FFT_N];
float   power[FFT_N];
float   angel[FFT_N];
UINT64  fft_out_data[FFT_N / 2];
UINT64  buffer_input[FFT_N];
UINT64  buffer_output[FFT_N];
/*********************************************************************************************************
** ��������: pthread_test
** ��������: FFT �������� pthread �߳�
** �䡡��  : pvArg         �̲߳���
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *pthread_test(VOID  *pvArg)
{
    INT         iFd;
    int         i;
    float       tempf1[3];
    fft_data_t *output_data;
    fft_data_t *input_data;

    fft_transaction_t  transaction;

    iFd = open("/dev/fft", O_RDWR, 0666);
    if (iFd < 0) {
        printf("failed to open /dev/fft \n");
        return  (LW_NULL);
    }

    printf("\n Going to test FFT module \n");

    for (i = 0; i < FFT_N; i++) {
        tempf1[0] = 0.3 * cosf(2 * PI * i / FFT_N + PI / 3) * 32768;
        tempf1[1] = 0.1 * cosf(16 * 2 * PI * i / FFT_N - PI / 9) * 32768;
        tempf1[2] = 0.5 * cosf(19 * 2 * PI * i / FFT_N + PI / 6) * 32768;
        real[i]   = tempf1[0] + tempf1[1] + tempf1[2];
        imag[i]   = 0;
    }

    for (i = 0; i < FFT_N / 2; ++i) {
        input_data     = (fft_data_t *)&buffer_input[i];
        input_data->R1 = real[2 * i];
        input_data->I1 = imag[2 * i];
        input_data->R2 = real[2 * i + 1];
        input_data->I2 = imag[2 * i + 1];
    }

    transaction.direction = FFT_DIR_FORWARD;
    transaction.input     = buffer_input;
    transaction.output    = buffer_output;
    transaction.point_num = FFT_N;

    ioctl(iFd, START_FFT_TRANSACTION, &transaction);

    for (i = 0; i < FFT_N / 2; ++i) {
        output_data     = (fft_data_t *)&buffer_output[i];
        real[2 * i]     = output_data->R1;
        imag[2 * i]     = output_data->I1;
        real[2 * i + 1] = output_data->R2;
        imag[2 * i + 1] = output_data->I2;
    }

    for (i = 0; i < FFT_N; i++) {
        power[i] = sqrt(real[i] * real[i] + imag[i] * imag[i]) * 2;
    }

    printf("\n phase: \n");
    for (i = 0; i < 32; i++) {
        angel[i] = atan2(imag[i], real[i]);
        printf("%d: %f \n", i, angel[i] * 180 / PI);
    }

    printf("\n power: \n");
    for (i = 0; i < 32; i++) {
        printf("%d: %f \n", i, power[i]);
    }

    for (i = 0; i < FFT_N / 2; ++i) {
        input_data     = (fft_data_t *)&buffer_input[i];
        input_data->R1 = real[2 * i];
        input_data->I1 = imag[2 * i];
        input_data->R2 = real[2 * i + 1];
        input_data->I2 = imag[2 * i + 1];
    }

    transaction.direction = FFT_DIR_BACKWARD;
    ioctl(iFd, START_FFT_TRANSACTION, &transaction);

    for (i = 0; i < FFT_N / 2; ++i) {
       output_data     = (fft_data_t *)&buffer_output[i];
       real[2 * i]     = output_data->R1;
       imag[2 * i]     = output_data->I1;
       real[2 * i + 1] = output_data->R2;
       imag[2 * i + 1] = output_data->I2;
    }

    for (i = 0; i < FFT_N / 2; ++i) {
        printf("%d: %d", i, real[i]);

        if (imag[i] >= 0) {
            printf("+%dj \n", imag[i]);
        } else if (imag[i] == 0) {
            printf("\n");
        } else {
            printf("%dj \n", imag[i]);
        }
    }

    close(iFd);

    return  (NULL);
}
/*********************************************************************************************************
** ��������: fftTestStart
** ��������: FFT ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  fftTestStart (VOID)
{
    pthread_t   tid;
    INT         iError;

    iError  = pthread_create(&tid, NULL, pthread_test, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
