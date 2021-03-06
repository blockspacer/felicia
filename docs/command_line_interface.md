# Command Line Interface

On one shell, you need to run [master_server_main](/docs/master_server_main.md).

On the other shell, now you should build the cli.

```bash
bazel build //felicia/core/master/tool/master_client_cli
```

And then you can use cli tool! Here belows are commands supported. For example, if you want to know what nodes are registered to the master, you can type like below.

```bash
bazel-bin/felicia/core/master/tool/master_client_cli node ls -a
```

* Client command

| COMMAND  | OPTION                  | DESCRIPTION                  |
| -------: | ----------------------: | ---------------------------: |
| ls       | -a, -all                | List all the clients         |
|          | --id                    | List clients with a given id |

* Node command

| COMMAND  | OPTION                  | DESCRIPTION                                     |
| -------: | ----------------------: | ----------------------------------------------: |
| ls       | -a, -all                | List all the nodes                              |
|          | -p, --publishing_topic  | List nodes publishing a given topic             |
|          | -s, --subscribing_topic | List nodes subscribing a given topic            |
|          | -n, --name              | List a node whose name is equal to a given name |

* Service command

| COMMAND   | OPTION                  | DESCRIPTION                                               |
| --------: | ----------------------: | --------------------------------------------------------: |
| ls        | -a, -all                | List all the services                                     |
|           | -s, --service           | List a given service                                      |

* Topic command

| COMMAND   | OPTION                  | DESCRIPTION                                               |
| --------: | ----------------------: | --------------------------------------------------------: |
| ls        | -a, -all                | List all the topics                                       |
|           | -t, --topic             | List a given topic                                        |
| publish   | topic                   | Content to publish                                        |
|           | message_type            | Type of message                                           |
|           | message                 | Content of message, in JSON format                        |
|           | -c, --channel           | Protocol to deliver message                               |
|           | -i, --interval          | Interval between messages, in milliseconds, default: 1000 |
| subscribe | -a, -all                | Subscribe all the topics                                  |
|           | -t, --topic             | Topic to subscribe                                        |
|           | -i, --interval          | Interval between messages, in milliseconds, default: 1000 |
|           | -q, --queue_size        | Queue size for each subsciber, default 10                 |

