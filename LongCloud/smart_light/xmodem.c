#include "hsf.h"
#include "hfsys.h"
#include "crc16.h"
#include "xmodem.h"
#include "uart_protocal.h"


#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A

#define DLY_1S 1000
#define MAXRETRANS 25

void _outbyte(int c)
{
	unsigned char ch;

	ch = c &0xFF;
	hfuart_send(HFUART1, &ch, 1,1000);
}



int _inbyte(unsigned short timeout) // msec timeout
{
	unsigned char ch;


	return -2;
}

unsigned short crc16_ccitt( const void *buf, int len )
{
    unsigned short crc = 0;
    while( len-- ) {
        int i;
        crc ^= *(char *)buf++ << 8;
        for( i = 0; i < 8; ++i ) {
            if( crc & 0x8000 )
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

static int check(int crc, const unsigned char *buf, int sz)
{
    if (crc) {
        unsigned short crc = crc16_ccitt(buf, sz);
        unsigned short tcrc = (buf[sz]<<8)+buf[sz+1];
        if (crc == tcrc)
            return 1;
    }
    else {
        int i;
        unsigned char cks = 0;
        for (i = 0; i < sz; ++i) {
            cks += buf[i];
        }
        if (cks == buf[sz])
        return 1;
    }

    return 0;
}

static void flushinput(void)
{
  //  while (_inbyte(((DLY_1S)*3)>>1) >= 0)
  //      ;
}

int xmodemReceive(unsigned char *dest, int destsz)
{
    unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
    unsigned char *p;
    int bufsz, crc = 0;
    unsigned char trychar = 'C';
    unsigned char packetno = 1;
    int i, c, len = 0;
    int retry, retrans = MAXRETRANS;

    for(;;) {
        for( retry = 0; retry < 16; ++retry) {
            if (trychar) _outbyte(trychar);
            if ((c = _inbyte((DLY_1S)<<1)) >= 0) {
                switch (c) {
                case SOH:
                    bufsz = 128;
                    goto start_recv;
                case STX:
                    bufsz = 1024;
                    goto start_recv;
                case EOT:
                    flushinput();
                    _outbyte(ACK);
                    return len; /* normal end */
                case CAN:
                    if ((c = _inbyte(DLY_1S)) == CAN) {
                        flushinput();
                        _outbyte(ACK);
                        return -1; /* canceled by remote */
                    }
                    break;
                default:
                    break;
                }
            }
        }
        if (trychar == 'C') { trychar = NAK; continue; }
        flushinput();
        _outbyte(CAN);
        _outbyte(CAN);
        _outbyte(CAN);
        return -2; /* sync error */

    start_recv:
        if (trychar == 'C') crc = 1;
        trychar = 0;
        p = xbuff;
        *p++ = c;
        for (i = 0;  i < (bufsz+(crc?1:0)+3); ++i) {
            if ((c = _inbyte(DLY_1S)) < 0) goto reject;
            *p++ = c;
        }

        if (xbuff[1] == (unsigned char)(~xbuff[2]) && 
            (xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno-1) &&
            check(crc, &xbuff[3], bufsz)) {
            if (xbuff[1] == packetno)    {
                register int count = destsz - len;
                if (count > bufsz) count = bufsz;
                if (count > 0) {
                    memcpy (&dest[len], &xbuff[3], count);
                    len += count;
                }
                ++packetno;
                retrans = MAXRETRANS+1;
            }
            if (--retrans <= 0) {
                flushinput();
                _outbyte(CAN);
                _outbyte(CAN);
                _outbyte(CAN);
                return -3; /* too many retry error */
            }
            _outbyte(ACK);
            continue;
        }
    reject:
        flushinput();
        _outbyte(NAK);
    }
}

void copy_xmodem_data(unsigned char *dest,uint32_t src,int len)
{
	hfuflash_read(src,dest,len);
}

int xmodemTransmit(uint32_t addr, int srcsz)
{
    unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
    int bufsz, crc = -1;
    unsigned char packetno = 1;
    int i, c, len = 0;
    int retry;

    for(;;) {
        for( retry = 0; retry < 16; ++retry) {
            if ((c = _inbyte((DLY_1S)<<1)) >= 0) {
                switch (c) {
                case 'C':
                    crc = 1;
                    goto start_trans;
                case NAK:
                    crc = 0;
                    goto start_trans;
                case CAN:
                    if ((c = _inbyte(DLY_1S)) == CAN) {
                        _outbyte(ACK);
                        flushinput();
                        return -1; /* canceled by remote */
                    }
                    break;
                default:
                    break;
                }
            }
        }
        _outbyte(CAN);
        _outbyte(CAN);
        _outbyte(CAN);
        flushinput();
        return -2; /* no sync */

        for(;;) {
        start_trans:
            xbuff[0] = SOH; bufsz = 128;
            xbuff[1] = packetno;
            xbuff[2] = ~packetno;
            c = srcsz - len;
            if (c > bufsz) c = bufsz;
            if (c >= 0) {
                memset (&xbuff[3], 0, bufsz);
                if (c == 0) {
                    xbuff[3] = CTRLZ;
                }
                else {
                    //memcpy (&xbuff[3], &src[len], c);
                    //copy_xmodem_data(&xbuff[3],addr+len,c);                    
					hfuflash_read(addr+len,&xbuff[3],len);
                    if (c < bufsz) xbuff[3+c] = CTRLZ;
                }
                if (crc) {
                    unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
                    xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
                    xbuff[bufsz+4] = ccrc & 0xFF;
                }
                else {
                    unsigned char ccks = 0;
                    for (i = 3; i < bufsz+3; ++i) {
                        ccks += xbuff[i];
                    }
                    xbuff[bufsz+3] = ccks;
                }
                for (retry = 0; retry < MAXRETRANS; ++retry) {
                    for (i = 0; i < bufsz+4+(crc?1:0); ++i) {
                        _outbyte(xbuff[i]);
                    }
                    if ((c = _inbyte(DLY_1S)) >= 0 ) {
                        switch (c) {
                        case ACK:
                            ++packetno;
                            len += bufsz;
                            goto start_trans;
                        case CAN:
                            if ((c = _inbyte(DLY_1S)) == CAN) {
                                _outbyte(ACK);
                                flushinput();
                                return -1; /* canceled by remote */
                            }
                            break;
                        case NAK:
                        default:
                            break;
                        }
                    }
                }
                _outbyte(CAN);
                _outbyte(CAN);
                _outbyte(CAN);
                flushinput();
                return -4; /* xmit error */
            }
            else {
                for (retry = 0; retry < 10; ++retry) {
                    _outbyte(EOT);
                    if ((c = _inbyte((DLY_1S)<<1)) == ACK) break;
                }
                flushinput();
                return (c == ACK)?len:-5;
            }
        }
    }
}

PROCESS(mcu_upgrade_process, "mcu_upgrade_process");

uart_buffer_t g_mcu_uart1;

int USER_FUNC uart1_mcu_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len)
{
	//hfuart_send(HFUART1, data, len,1000);	

	Push_data(&g_mcu_uart1,data, len);

	return 0;
}

unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
int g_crc= -1;
unsigned char g_packetno = 1;
int g_retry;
uint32_t g_addr=4096;
int g_len = 0;
int g_ret=0;
int g_srcsz;
int g_bufsz;
int g_firstcan=0;
int g_start = 0;
int g_c;
int g_endretry=0;
int v_do=1;


int Xmodem_trans_onframe(void)
{
	int i,c;
	
	xbuff[0] = SOH; g_bufsz = 128;
	xbuff[1] = g_packetno;
	xbuff[2] = ~g_packetno;
	c = g_srcsz - g_len;
	//hfuart_send(HFUART1, &c, 2,1000);
	if (c > g_bufsz) c = g_bufsz;	
	if (c >= 0) 
	{
		memset (&xbuff[3], 0, g_bufsz);
		if (c == 0) 
		{
			xbuff[3] = CTRLZ;
		}
		else 
		{
			//memcpy (&xbuff[3], &src[len], c);
			//copy_xmodem_data(&xbuff[3],addr+len,c);
			hfuflash_read(g_addr+g_len,&xbuff[3],c);					  
			if (c < g_bufsz) xbuff[3+c] = CTRLZ;
		}
		if (g_crc) 
		{
			unsigned short ccrc = crc16_ccitt(&xbuff[3], g_bufsz);
			xbuff[g_bufsz+3] = (ccrc>>8) & 0xFF;
			xbuff[g_bufsz+4] = ccrc & 0xFF;
		}
		else 
		{
			unsigned char ccks = 0;
			for (i = 3; i < g_bufsz+3; ++i) 
			{
				ccks += xbuff[i];
			}
			xbuff[g_bufsz+3] = ccks;
		}
		g_endretry=25;
		for (i = 0; i < g_bufsz+4+(g_crc?1:0); ++i) 
		{
			_outbyte(xbuff[i]);
		}
	}
	else
	{
		g_start = 2;
		g_endretry = 10;
	}

	return 0;
}

#define  UART_WAITING_TIME		5000

PROCESS_THREAD(mcu_upgrade_process, ev, data)
{
	PROCESS_BEGIN();
	
    static struct etimer v_timer_sleep;
	unsigned char ch;
	
	memset(&g_mcu_uart1,0,sizeof(uart_buffer_t));
	
	if(hfnet_start_uart1_ex(0,uart1_mcu_recv_callback,1024) != HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start uart fail!\n");
	}	
	etimer_set(&v_timer_sleep, UART_WAITING_TIME);

	while(v_do)
	{
		
		if( g_start == 2)
		{
			_outbyte(EOT);
		}
		g_ret = 0;
		
		while(1)
		{
			if(Pop_data(&g_mcu_uart1, g_mcu_uart1.r_index,&ch, 1, 0) == 0)
			{
				PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER );
		        if(ev == PROCESS_EVENT_TIMER)
		        {
					g_ret++;

					if(g_ret < 200)
					{
						etimer_set(&v_timer_sleep, UART_WAITING_TIME);	
						continue;
					}
					g_c = -2;
					etimer_set(&v_timer_sleep, UART_WAITING_TIME);						
					break;

		        }				
			}
			else
			{
				g_c = ch & 0x0ff;
				etimer_set(&v_timer_sleep, UART_WAITING_TIME);					
				break;
			}
		}
		//hfuart_send(HFUART1, &g_c, 1,1000);

		if( g_c == 'C')
		{
			//start xmodem
			g_crc = 1;
			g_start = 1;
			Xmodem_trans_onframe();
		}
		else if( g_c == ACK )
		{
			//应答
			if( g_start == 1)
			{
	            ++g_packetno;
	   	         g_len += g_bufsz;	
				Xmodem_trans_onframe();
			}
			else if(g_start == 2)
			{
				v_do = 0;
	            flushinput();	
			}
		}
		else if(g_c == NAK )
		{
			if(g_start == 0 )
			{
				g_crc = 0;
				g_start = 1;
				Xmodem_trans_onframe();
			}
			else
			{
				v_do = 0;
	            _outbyte(CAN);
	            _outbyte(CAN);
	            _outbyte(CAN);
	            flushinput();	
			}
		}
		else if(g_c == CAN)
		{
			if(g_firstcan ==0)
				g_firstcan = 1;
			else if(g_firstcan == 1)
			{	
				_outbyte(ACK);
				v_do = 0;
                _outbyte(CAN);
                _outbyte(CAN);
                _outbyte(CAN);
                flushinput();				
			}
		}
		else  //Nothing
		{
			if( g_start == 2)
			{
				g_endretry --;
				if(g_endretry == 0)
				{
					v_do = 0;
					_outbyte(CAN);
					_outbyte(CAN);
					_outbyte(CAN);
					flushinput();								
				}
			}
			else if(g_start == 1)
			{
				//Resend
				Xmodem_trans_onframe();
				g_endretry --;
				if(g_endretry == 0)
				{
					v_do = 0;
					_outbyte(CAN);
					_outbyte(CAN);
					_outbyte(CAN);
					flushinput();								
				}				
			}
			else if(g_start == 0)
			{
				g_retry --;
				if(g_retry == 0)
				{
					v_do = 0;
					_outbyte(CAN);
					_outbyte(CAN);
					_outbyte(CAN);
					flushinput();							
				}
			}
			else
			{
				
				v_do = 0;
	            _outbyte(CAN);
	            _outbyte(CAN);
	            _outbyte(CAN);
	            flushinput();			
			}
		}
	}

	if(hfnet_start_uart1_ex(0,uart1_recv_callback,1024) != HF_SUCCESS)
	{
		HF_Debug(DEBUG_WARN,"start uart fail!\n");
	}
	etimer_stop(&v_timer_sleep);
	//hfuart_send(HFUART1, "OVER", 4,1000);

	PROCESS_END();
}


void mcu_upgrade_start(void)
{
	hfuflash_read(0,&g_srcsz,4);
	//hfuart_send(HFUART1, &g_srcsz, 4,1000);
	//xmodemTransmit(4096, srcsz);	
	g_addr = 4096;
	g_crc = -1;
	g_len = 0;
	g_ret = 0;
	g_packetno = 1;
	g_firstcan = 0;
	g_start = 0;
	g_endretry=0;
	g_retry = 16;
	v_do = 1;
    process_start(&mcu_upgrade_process, NULL);
}



