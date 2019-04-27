#ifndef __CLUSTER_H
#define __CLUSTER_H

/*-----------------------------------------------------------------------------
 * Redis cluster data structures, defines, exported API.
 *----------------------------------------------------------------------------*/

#define CLUSTER_SLOTS 16384
#define CLUSTER_OK 0          /* Everything looks ok */
#define CLUSTER_FAIL 1        /* The cluster can't work */
#define CLUSTER_NAMELEN 40    /* sha1 hex length */
#define CLUSTER_PORT_INCR 10000 /* Cluster port = baseport + PORT_INCR */

/* The following defines are amount of time, sometimes expressed as
 * multiplicators of the node timeout value (when ending with MULT). */
#define CLUSTER_DEFAULT_NODE_TIMEOUT 15000
#define CLUSTER_DEFAULT_SLAVE_VALIDITY 10 /* Slave max data age factor. */
#define CLUSTER_DEFAULT_REQUIRE_FULL_COVERAGE 1
#define CLUSTER_DEFAULT_SLAVE_NO_FAILOVER 0 /* Failover by default. */
#define CLUSTER_FAIL_REPORT_VALIDITY_MULT 2 /* Fail report validity. */
#define CLUSTER_FAIL_UNDO_TIME_MULT 2 /* Undo fail if master is back. */
#define CLUSTER_FAIL_UNDO_TIME_ADD 10 /* Some additional time. */
#define CLUSTER_FAILOVER_DELAY 5 /* Seconds */
#define CLUSTER_DEFAULT_MIGRATION_BARRIER 1
#define CLUSTER_MF_TIMEOUT 5000 /* Milliseconds to do a manual failover. */
#define CLUSTER_MF_PAUSE_MULT 2 /* Master pause manual failover mult. */
#define CLUSTER_SLAVE_MIGRATION_DELAY 5000 /* Delay for slave migration. */

/* Redirection errors returned by getNodeByQuery(). */
#define CLUSTER_REDIR_NONE 0          /* Node can serve the request. */
#define CLUSTER_REDIR_CROSS_SLOT 1    /* -CROSSSLOT request. */
#define CLUSTER_REDIR_UNSTABLE 2      /* -TRYAGAIN redirection required */
#define CLUSTER_REDIR_ASK 3           /* -ASK redirection required. */
#define CLUSTER_REDIR_MOVED 4         /* -MOVED redirection required. */
#define CLUSTER_REDIR_DOWN_STATE 5    /* -CLUSTERDOWN, global state. */
#define CLUSTER_REDIR_DOWN_UNBOUND 6  /* -CLUSTERDOWN, unbound slot. */

struct clusterNode;

/*
 * clusterLink encapsulates everything needed to talk with a remote node.
 * clusterLink封装了与远程节点通信所需的一切。
 */
typedef struct clusterLink {
    // 连接的创建时间
    mstime_t ctime;             /* Link creation time */
    // TCP 套接字描述符
    int fd;                     /* TCP socket file descriptor */
    // 输出缓冲区，保存着等待发送给其他节点的消息
    sds sndbuf;                 /* Packet send buffer */
    // 输入缓冲区，保存着从其它节点受到的消息
    sds rcvbuf;                 /* Packet reception buffer */
    // 与这个连接相关联的节点，如果没有的话就为 null
    struct clusterNode *node;   /* Node related to this link if any, or NULL */
} clusterLink;

/* Cluster node flags and macros. 群集节点标志和宏。 */
#define CLUSTER_NODE_MASTER 1     /* The node is a master */
#define CLUSTER_NODE_SLAVE 2      /* The node is a slave */
#define CLUSTER_NODE_PFAIL 4      /* 主观下线状态 */
#define CLUSTER_NODE_FAIL 8       /* 客观下线状态 */
#define CLUSTER_NODE_MYSELF 16    /* 标识自身节点 */
#define CLUSTER_NODE_HANDSHAKE 32 /* 握手状态，未与其他节点进行消息通信 */
#define CLUSTER_NODE_NOADDR   64  /* 无地址节点，用于第一次 meet 通信未完成或者通信失败 */
#define CLUSTER_NODE_MEET 128     /* 需要接收 meet 消息的节点状态 */
#define CLUSTER_NODE_MIGRATE_TO 256 /* 该节点被选中为新的主节点状态 */
#define CLUSTER_NODE_NOFAILOVER 512 /* Slave will not try to failver. */
#define CLUSTER_NODE_NULL_NAME "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"

#define nodeIsMaster(n) ((n)->flags & CLUSTER_NODE_MASTER)
#define nodeIsSlave(n) ((n)->flags & CLUSTER_NODE_SLAVE)
#define nodeInHandshake(n) ((n)->flags & CLUSTER_NODE_HANDSHAKE)
#define nodeHasAddr(n) (!((n)->flags & CLUSTER_NODE_NOADDR))
#define nodeWithoutAddr(n) ((n)->flags & CLUSTER_NODE_NOADDR)
#define nodeTimedOut(n) ((n)->flags & CLUSTER_NODE_PFAIL)
#define nodeFailed(n) ((n)->flags & CLUSTER_NODE_FAIL)
#define nodeCantFailover(n) ((n)->flags & CLUSTER_NODE_NOFAILOVER)

/* Reasons why a slave is not able to failover. */
#define CLUSTER_CANT_FAILOVER_NONE 0
#define CLUSTER_CANT_FAILOVER_DATA_AGE 1
#define CLUSTER_CANT_FAILOVER_WAITING_DELAY 2
#define CLUSTER_CANT_FAILOVER_EXPIRED 3
#define CLUSTER_CANT_FAILOVER_WAITING_VOTES 4
#define CLUSTER_CANT_FAILOVER_RELOG_PERIOD (60*5) /* seconds. */

/* clusterState todo_before_sleep flags. */
#define CLUSTER_TODO_HANDLE_FAILOVER (1<<0)
#define CLUSTER_TODO_UPDATE_STATE (1<<1)
#define CLUSTER_TODO_SAVE_CONFIG (1<<2)
#define CLUSTER_TODO_FSYNC_CONFIG (1<<3)

/* Message types.
 *
 * Note that the PING, PONG and MEET messages are actually the same exact
 * kind of packet. PONG is the reply to ping, in the exact format as a PING,
 * while MEET is a special PING that forces the receiver to add the sender
 * as a node (if it is not already in the list). */
#define CLUSTERMSG_TYPE_PING 0          /* Ping */
#define CLUSTERMSG_TYPE_PONG 1          /* Pong (reply to Ping) */
#define CLUSTERMSG_TYPE_MEET 2          /* Meet "let's join" message */
#define CLUSTERMSG_TYPE_FAIL 3          /* Mark node xxx as failing */
#define CLUSTERMSG_TYPE_PUBLISH 4       /* Pub/Sub Publish propagation */
#define CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST 5 /* May I failover? */
#define CLUSTERMSG_TYPE_FAILOVER_AUTH_ACK 6     /* Yes, you have my vote */
#define CLUSTERMSG_TYPE_UPDATE 7        /* Another node slots configuration */
#define CLUSTERMSG_TYPE_MFSTART 8       /* Pause clients for manual failover */
#define CLUSTERMSG_TYPE_MODULE 9        /* Module cluster API message. */
#define CLUSTERMSG_TYPE_COUNT 10        /* Total number of message types. */

/* This structure represent elements of node->fail_reports. */
typedef struct clusterNodeFailReport {
    /**
     * 报告目标节点已经下线的节点
     */
    struct clusterNode *node;  /* Node reporting the failure condition. */
    /**
     * 最后一次从 node 节点收到下线报告的时间
     * 程序使用这个时间戳来检测下线报告是否过期
     * （与当前时间相差太久的下线报告会被删除）
     */
    mstime_t time;             /* Time of the last report from this node. */
} clusterNodeFailReport;

/**
 * clusterState.slots 和 clusterNode.slots 的区别
 *
 * clusterState.slots 数组记录了集群中所有槽的指派信息，
 * 而 clusterNode.slots 数组只记录了 clusterNode 结构所代表的节点的槽指派信息
 *
 */

/**
 * 记录节点的槽指派信息
 */
typedef struct clusterNode {
    // 节点创建的时间
    mstime_t ctime; /* Node object creation time. */
    /**
     * 节点的名字,由 40个十六进制字符组成
     */
    char name[CLUSTER_NAMELEN]; /* Node name, hex string, sha1-size */
    /**
     * 节点标识
     * 使用各种不同的标识值记录节点的角色(比如主节点或者从节点)
     * 以及节点目前所处的状态(比如在线或者下线)
     */
    int flags;      /* CLUSTER_NODE_... */
    uint64_t configEpoch; /* Last configEpoch observed for this node */

    // slots 是一个二进制位数组，长度为 16384/8=2048 bit。共包含 16384 个二进制位
    // 如果 slots 数组在索引 i 上的二进制位的值为 1，则表示该节点负责处理槽i
    /**
     * 取出和设置 slots 数组中的任意一个二进制位的值得复杂度都是 O(1)
     */
    unsigned char slots[CLUSTER_SLOTS/8]; /* slots handled by this node */
    /**
     * 该节点处理 slot 的个数
     */
    int numslots;   /* Number of slots handled by this node */
    /**
     * 如果这是 master，则记录了 slaves 的数量
     */
    int numslaves;  /* Number of slave nodes, if this is a master */
    /**
     * 指向从节点的链表
     */
    struct clusterNode **slaves; /* pointers to slave nodes */
    struct clusterNode *slaveof; /* pointer to the master node. Note that it
                                    may be NULL even if the node is a slave
                                    if we don't have the master node in our
                                    tables. */
    // ping 命令发送的时间
    mstime_t ping_sent;      /* Unix time we sent latest ping */
    mstime_t pong_received;  /* Unix time we received the pong */
    // fail消息 发送的时间
    mstime_t fail_time;      /* Unix time when FAIL flag was set */
    // 发起投票的时间
    mstime_t voted_time;     /* Last time we voted for a slave of this master */
    mstime_t repl_offset_time;  /* Unix time we received offset for this node */
    mstime_t orphaned_time;     /* Starting time of orphaned master condition */
    long long repl_offset;      /* Last known repl offset for this node. */
    /**
     * 节点的ip 地址
     */
    char ip[NET_IP_STR_LEN];  /* Latest known IP address of this node */
    /**
     * 节点端口号
     */
    int port;                   /* Latest known clients port of this node */
    int cport;                  /* Latest known cluster port of this node. */
    /**
     * 保存连接节点所需要的有关信息
     */
    clusterLink *link;          /* TCP/IP link with this node */
    list *fail_reports;         /* List of nodes signaling this as failing */
} clusterNode;

typedef struct clusterState {
    /**
     * 指向当前节点的指针
     */
    clusterNode *myself;  /* This node */
    /**
     * 集群当前的配置纪元,用于实现故障转移
     */
    uint64_t currentEpoch;
    /**
     * 集群当前的状态:是在线还是下线
     */
    int state;            /* CLUSTER_OK, CLUSTER_FAIL, ... */
    /**
     * 集群中至少处理着一个槽的节点的数量
     */
    int size;             /* Num of master nodes with at least one slot */
    /**
     * 集群节点名单(包括 myself 节点)
     * 字典的键为节点的名字,字典的值为节点对应的 clusterNode 结构
     */
    dict *nodes;          /* Hash table of name -> clusterNode structures */
    dict *nodes_black_list; /* Nodes we don't re-add for a few seconds. */
    /*
     * 如果节点没有在自己的数据库里找到key，那么节点会检查自己的 clusterState.migrating_slots_to,
     * 查看 key 所属的槽 i 是否正在进行迁移，如果槽i 在迁移，则向客户端发送一个 ask 命令
     *
     * 该数组记录当前节点正在迁移至其它节点的槽
     */
    clusterNode *migrating_slots_to[CLUSTER_SLOTS];
    // 记录了当前节点正在从其它节点导入的槽
    clusterNode *importing_slots_from[CLUSTER_SLOTS];
    /**
     * slots 数组记录了集群中所有 16384 个槽的指派信息
     * 每个数组项都指向了 clusterNode 结构的指针：
     * 如果 slots[i] 指针指向了 null，那么标识该槽未指派给任何节点。
     */
    clusterNode *slots[CLUSTER_SLOTS];
    uint64_t slots_keys_count[CLUSTER_SLOTS];
    rax *slots_to_keys;
    /* The following fields are used to take the slave state on elections. */
    mstime_t failover_auth_time; /* Time of previous or next election. */
    int failover_auth_count;    /* Number of votes received so far. */
    int failover_auth_sent;     /* True if we already asked for votes. */
    int failover_auth_rank;     /* This slave rank for current auth request. */
    uint64_t failover_auth_epoch; /* Epoch of the current election. */
    int cant_failover_reason;   /* Why a slave is currently not able to
                                   failover. See the CANT_FAILOVER_* macros. */
    /* Manual failover state in common. */
    mstime_t mf_end;            /* Manual failover time limit (ms unixtime).
                                   It is zero if there is no MF in progress. */
    /* Manual failover state of master. */
    clusterNode *mf_slave;      /* Slave performing the manual failover. */
    /* Manual failover state of slave. */
    long long mf_master_offset; /* Master offset the slave needs to start MF
                                   or zero if stil not received. */
    int mf_can_start;           /* If non-zero signal that the manual failover
                                   can start requesting masters vote. */
    /* The followign fields are used by masters to take state on elections. */
    uint64_t lastVoteEpoch;     /* Epoch of the last vote granted. */
    int todo_before_sleep; /* Things to do in clusterBeforeSleep(). */
    /* Messages received and sent by type. */
    long long stats_bus_messages_sent[CLUSTERMSG_TYPE_COUNT];
    long long stats_bus_messages_received[CLUSTERMSG_TYPE_COUNT];
    long long stats_pfail_nodes;    /* Number of nodes in PFAIL status,
                                       excluding nodes without address. */
} clusterState;

/* Redis cluster messages header */

/* Initially we don't know our "name", but we'll find it once we connect
 * to the first node, using the getsockname() function. Then we'll use this
 * address for all the next messages. */
typedef struct {
    // 节点的名字
    char nodename[CLUSTER_NAMELEN];
    // 最后一次向该节点发送 PING 消息的时间戳
    uint32_t ping_sent;
    // 最后一次从该节点接收到 PONG 消息的时间戳
    uint32_t pong_received;
    // 节点的 IP 地址
    char ip[NET_IP_STR_LEN];  /* IP address last time it was seen */
    // 节点的端口号
    uint16_t port;              /* base port last time it was seen */
    uint16_t cport;             /* cluster port last time it was seen */
    // 节点的标识值
    uint16_t flags;             /* node->flags copy */
    uint32_t notused1;
} clusterMsgDataGossip;

/**
 * 这里就是 FAIL 的正文
 * todo: 为什么 FAIL 不使用 Gossip 协议？
 * 在集群的节点数量比较大的情况单纯使用 Gossip 协议来传播节点的已下线信息会给
 * 节点的信息更新带来一定延迟，因为 Gossip 协议消息通常需要一段时间才能传播至
 * 整个集群，而发送 FAIL 消息可以让集群里的所有节点立即知道某个主节点已下线，从而
 * 尽快判断是否需要将集群标记为下线，又或者对下线主节点进行故障转移。
 */
typedef struct {
    /*
     * todo: 因为集群里的所有节点都是独一无二的名字，
     * 所以 FAIL 消息里面只需要保存下线节点的名字
     * 接收到消息的节点就可以根据这个名字来判断是哪个节点下线了。
     */
    char nodename[CLUSTER_NAMELEN];
} clusterMsgDataFail;

typedef struct {
    uint32_t channel_len;
    uint32_t message_len;
    // 定义为 8 字节只是为了对齐其他消息结构
    // 实际的长度有保存的内容决定
    unsigned char bulk_data[8]; /* 8 bytes just as placeholder. */
} clusterMsgDataPublish;

typedef struct {
    uint64_t configEpoch; /* Config epoch of the specified instance. */
    char nodename[CLUSTER_NAMELEN]; /* Name of the slots owner. */
    unsigned char slots[CLUSTER_SLOTS/8]; /* Slots bitmap. */
} clusterMsgDataUpdate;

typedef struct {
    uint64_t module_id;     /* ID of the sender module. */
    uint32_t len;           /* ID of the sender module. */
    uint8_t type;           /* Type from 0 to 255. */
    unsigned char bulk_data[3]; /* 3 bytes just as placeholder. */
} clusterMsgModule;

/**
 * 消息的正文
 */
union clusterMsgData {
    /* PING, MEET and PONG 消息的正文 */
    struct {
        // 这里使用的 Gossip 协议，这几个命令中只有这三个命令使用 Gossip 协议
        /* Array of N clusterMsgDataGossip structures */
        clusterMsgDataGossip gossip[1];
    } ping;


    /* FAIL */
    struct {
        clusterMsgDataFail about;
    } fail;

    /**
     *
     */
    /* PUBLISH */
    struct {
        clusterMsgDataPublish msg;
    } publish;

    /* UPDATE */
    struct {
        clusterMsgDataUpdate nodecfg;
    } update;

    /* MODULE */
    struct {
        clusterMsgModule msg;
    } module;
};

#define CLUSTER_PROTO_VER 1 /* Cluster bus protocol version. */

/**
 * 消息头信息
 * 该结构中的 currentEpoch、sender、myslots 等属性纪录了发送者自身的节点信息，
 * 接收者会根据这些信息，在自己的 clusterState.nodes 字典里找到发送者对应的
 * clusterNode 结构，并被结构进行更新。
 */
typedef struct {
    char sig[4];        /* Siganture "RCmb" (Redis Cluster message bus). */
    // 消息的长度（包括这个消息头的长度和消息正文的长度）
    uint32_t totlen;    /* Total length of this message */
    //
    uint16_t ver;       /* Protocol version, currently set to 1. */
    uint16_t port;      /* TCP base port number. */
    // 消息的类型
    uint16_t type;      /* Message type */
    // 消息正文包含的节点信息数量
    // 只在发送 MEET、PING、PONG、这三种 Gossip 协议消息时使用
    uint16_t count;     /* Only used for some kind of messages. */
    // 发送者所处的配置纪元
    uint64_t currentEpoch;  /* The epoch accordingly to the sending node. */
    // 如果发送者是一个主节点，那么这里纪录的是发送者的配置纪元
    // 如果发送者是一个从节点，那么这里的是发送者正在复制的主节点的配置纪元
    uint64_t configEpoch;   /* The config epoch if it's a master, or the last
                               epoch advertised by its master if it is a
                               slave. */
    uint64_t offset;    /* Master replication offset if node is a master or
                           processed replication offset if node is a slave. */
    // 发送者的名字（ID）
    char sender[CLUSTER_NAMELEN]; /* Name of the sender node */
    // 发送者目前的槽指派信息
    unsigned char myslots[CLUSTER_SLOTS/8];
    // 如果发送者是一个从节点，那么这里纪录的是发送者正在复制的主节点的名字
    // 如果发送者是一个主节点，那么这里纪录的是 REDIS_NODE_NULL_NAME
    //（一个 40 字节长，值全为 0 的字节数组）
    char slaveof[CLUSTER_NAMELEN];
    char myip[NET_IP_STR_LEN];    /* Sender IP, if not all zeroed. */
    char notused1[34];  /* 34 bytes reserved for future usage. */
    // 发送者的端口号
    uint16_t cport;      /* Sender TCP cluster bus port */
    // 发送者的标识符
    uint16_t flags;      /* Sender node flags */
    // 发送者所处集群的状态
    unsigned char state; /* Cluster state from the POV of the sender */
    unsigned char mflags[3]; /* Message flags: CLUSTERMSG_FLAG[012]_... */
    // 消息的正文（内容）
    union clusterMsgData data;
} clusterMsg;

#define CLUSTERMSG_MIN_LEN (sizeof(clusterMsg)-sizeof(union clusterMsgData))

/* Message flags better specify the packet content or are used to
 * provide some information about the node state. */
#define CLUSTERMSG_FLAG0_PAUSED (1<<0) /* Master paused for manual failover. */
#define CLUSTERMSG_FLAG0_FORCEACK (1<<1) /* Give ACK to AUTH_REQUEST even if
                                            master is up. */

/* ---------------------- API exported outside cluster.c -------------------- */
clusterNode *getNodeByQuery(client *c, struct redisCommand *cmd, robj **argv, int argc, int *hashslot, int *ask);
int clusterRedirectBlockedClientIfNeeded(client *c);
void clusterRedirectClient(client *c, clusterNode *n, int hashslot, int error_code);

#endif /* __CLUSTER_H */
