#include <rtthread.h>
#include <finsh.h>
#include <dfs_posix.h>
#include "board.h"

#define CODEC_CMD_SAMPLERATE	2

//定义mempool块大小.
#define  mempll_block_size      16384
//我们共申请两块mempool,并留出4字节做为控制块.
static rt_uint8_t mempool[ (mempll_block_size+4) *2];
static struct rt_mempool _mp;
//内存池初始化标识
static rt_bool_t is_inited = RT_FALSE;

static rt_err_t wav_tx_done(rt_device_t dev, void *buffer)
{
    /* release memory block */
    rt_mp_free(buffer);

    return RT_EOK;
}

struct RIFF_HEADER_DEF
{
    char riff_id[4];     // 'R','I','F','F'
    uint32_t riff_size;
    char riff_format[4]; // 'W','A','V','E'
};

struct WAVE_FORMAT_DEF
{
    uint16_t FormatTag;
    uint16_t Channels;
    uint32_t SamplesPerSec;
    uint32_t AvgBytesPerSec;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
};

struct FMT_BLOCK_DEF
{
    char fmt_id[4];    // 'f','m','t',' '
    uint32_t fmt_size;
    struct WAVE_FORMAT_DEF wav_format;
};

ALIGN(4)
static uint8_t header_buffer[100];
static rt_err_t wav_header_gen(int fd, rt_size_t size)
{
    struct RIFF_HEADER_DEF * riff_header;
    struct FMT_BLOCK_DEF *   fmt_block;
    rt_size_t offset = 0;

    memset(&header_buffer, 0, sizeof(header_buffer));

    riff_header = (struct RIFF_HEADER_DEF *)&header_buffer[offset];
    offset += sizeof(struct RIFF_HEADER_DEF);

    fmt_block = (struct FMT_BLOCK_DEF *)&header_buffer[offset];
    offset += sizeof(struct FMT_BLOCK_DEF);

    strncpy(riff_header->riff_id, "RIFF", 4);
    riff_header->riff_size = size - 8;
    strncpy(riff_header->riff_format, "WAVE", 4);

    strncpy(fmt_block->fmt_id, "fmt ", 4);
    fmt_block->fmt_size = sizeof(struct WAVE_FORMAT_DEF);
    fmt_block->wav_format.FormatTag = 1;
    fmt_block->wav_format.Channels = 2;
    fmt_block->wav_format.SamplesPerSec = 44100;
    fmt_block->wav_format.BlockAlign = 4;
    fmt_block->wav_format.BitsPerSample = 16;
    fmt_block->wav_format.AvgBytesPerSec = fmt_block->wav_format.SamplesPerSec
                                           * fmt_block->wav_format.Channels
                                           * fmt_block->wav_format.BitsPerSample
                                           / 8;

    strncpy((char*)&header_buffer[offset], "data", 4);
    offset += 4;
    {
        uint32_t * data_size = (uint32_t *)&header_buffer[offset];
        offset += 4;
        *data_size = size - offset;
    }


    lseek(fd, 0, DFS_SEEK_SET);
    write(fd, header_buffer, offset);
	
	return RT_EOK;
}

void wav(char* filename)
{
    int fd;
    struct FMT_BLOCK_DEF   fmt_block;

    //检查mempool是否被初始化,否则进行初始化.
    if (is_inited == RT_FALSE)
    {
        rt_mp_init(&_mp, "wav_buf", &mempool[0], sizeof(mempool), mempll_block_size);
        is_inited = RT_TRUE;
    }


    //打开文件
    fd = open(filename, O_RDONLY, 0);
    if (fd >= 0)
    {
        rt_uint8_t* buf;
        rt_size_t 	len;
		rt_device_t player;
        char riff_chunk[4];

        /* wav format check */
        do
        {
            len = read(fd, riff_chunk, sizeof(riff_chunk));
            if(len != sizeof(riff_chunk))
            {
                rt_kprintf("read riff chunk fail!\r\n");
                return;
            }

            if(strncmp(riff_chunk, "RIFF", sizeof(riff_chunk)) == 0)
            {
                struct RIFF_HEADER_DEF riff_header;

                /* read riff header */
                len = read(fd, (void*)((uint32_t)&riff_header + sizeof(riff_chunk)),
                           sizeof(struct RIFF_HEADER_DEF) - sizeof(riff_chunk));
                if(strncmp(riff_header.riff_format, "WAVE", 4) != 0)
                {
                    rt_kprintf("RIFF format error:%-4s\r\n", riff_header.riff_format);
                    return;
                }
            }
            else if(strncmp(riff_chunk, "fmt ", sizeof(riff_chunk)) == 0)
            {
                /* read riff format block */
                len = read(fd, (void*)((uint32_t)&fmt_block + sizeof(riff_chunk)),
                           sizeof(struct FMT_BLOCK_DEF) - sizeof(riff_chunk));
                if(len != sizeof(struct FMT_BLOCK_DEF) - sizeof(riff_chunk))
                {
                    rt_kprintf("read riff format block fail!\r\n");
                    return;
                }

                if(fmt_block.fmt_size != 16)
                {
                    char tmp[2];
                    read(fd, tmp, fmt_block.fmt_size - 16);
                }

                if(fmt_block.wav_format.Channels != 2)
                {
                    rt_kprintf("[err] only support stereo!\r\n");
                    return;
                }
            }
        }
        while(strncmp(riff_chunk, "data", 4) != 0);

        /* get data size */
        {
            rt_size_t size;
            len = read(fd, &size, 4);
            if(len != 4)
            {
                rt_kprintf("read data size fail!\r\n");
                return;
            }

            /* print paly time */
            {
                uint32_t hour, min, sec;

                hour = min = 0;
                sec = size / fmt_block.wav_format.AvgBytesPerSec;

                if(sec / (60*60))
                {
                    hour = sec / (60*60);
                    sec -= hour * (60*60);
                }

                if(sec / 60)
                {
                    min = sec / 60;
                    sec -= min * 60;
                }

                /* dump wav info, (only in finsh) */
                if(strncmp(rt_thread_self()->name, "tshell", sizeof("tshell") -1) == 0)
                {
                    rt_kprintf("wav info:\r\n");
                    rt_kprintf("Channels:%d ", fmt_block.wav_format.Channels);
                    rt_kprintf("SamplesPerSec:%d ", fmt_block.wav_format.SamplesPerSec);
                    rt_kprintf("BitsPerSample:%d\r\n", fmt_block.wav_format.BitsPerSample);
                    rt_kprintf("paly time: %02d:%02d:%02d\r\n", hour, min, sec);
                }
            }
        } /* get data size */

        /* open audio player and set tx done call back */
        player = rt_device_find("snd");
        if(player == RT_NULL)
        {
            rt_kprintf("audio player not found!\r\n");
            return;
        }

        /* set samplerate */
        {
            int SamplesPerSec = fmt_block.wav_format.SamplesPerSec;
            if(rt_device_control(player, CODEC_CMD_SAMPLERATE, &SamplesPerSec)
                    != RT_EOK)
            {
                rt_kprintf("audio player doesn't support this sample rate: %d\r\n",
                           SamplesPerSec);
                return;
            }
        }

        //设置发送完成回调函数,让DAC数据发完时执行wav_tx_done函数释放空间.
        rt_device_set_tx_complete(player, wav_tx_done);
        rt_device_open(player, RT_DEVICE_OFLAG_WRONLY);

        do
        {
            //向mempoll申请空间,如果申请不成功则一直在此等待.
            buf = rt_mp_alloc(&_mp, RT_WAITING_FOREVER);
            //从文件读取数据
            len = read(fd, (char*)buf, mempll_block_size);
            //读取成功就把数据写入设备
            if (len > 0)
            {
                rt_device_write(player, 0, buf, len);
            }
            //否则释放刚才申请的空间,正常情况下是读到文件尾时.
            else
            {
                rt_mp_free(buf);
            }
        }
        while (len != 0);

        /* close player and file */
        rt_device_close(player);
        close(fd);
    }
}
FINSH_FUNCTION_EXPORT(wav, wav test. e.g: wav("/test.wav"))

/*录音*/
#include "codec_wm8978_i2c.h"
#define TEST_FN		"/rec.wav"
uint8_t rx_buf[RX_BUFF_SIZE];
void wav_rec(void)
{
	rt_device_t record;
	
	uint32_t rx_size;
	uint32_t count = 0;
	int fd;
	
	record=rt_device_find("snd");
	if(record==0)
	{
		rt_kprintf("audio device not found!\r\n");
		return ;
	}
	
	/* create file. */
	fd = open(TEST_FN, O_WRONLY | O_CREAT | O_TRUNC, 0);
	if (fd < 0)
	{
		rt_kprintf("open file for write failed\n");
		return;
	}
	
	record->init(record);
	record->open(record,RT_DEVICE_OFLAG_RDONLY);
	rt_kprintf("REC start!\r\n");
	while (1)
	{

		rx_size=record->read(record,0,rx_buf,RX_BUFF_SIZE);
		/* write to file */
		write(fd,rx_buf,rx_size);

		count++;		
		if (count > 2000)
		{
			record->close(record);
			/* add header */
			wav_header_gen(fd, RX_BUFF_SIZE * count);
			rt_kprintf("REC done, count: %u\r\n", count);

			close(fd);
			break;
		}
	}
}

FINSH_FUNCTION_EXPORT(wav_rec, record test)
