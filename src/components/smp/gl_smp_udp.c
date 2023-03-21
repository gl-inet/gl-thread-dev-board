/*****************************************************************************
 * @file  gl_smp_udp.c
 * @brief Provide smp of udp related functions.
 *******************************************************************************
 Copyright 2022 GL-iNet. https://www.gl-inet.com/

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ******************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/smp_udp.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_conn_mgr.h>

#include <zephyr/logging/log.h>

#include "gl_smp_udp.h"

LOG_MODULE_REGISTER(gl_smp_udp, CONFIG_GL_SMP_UDP_LOG_LEVEL);

#define EVENT_MASK (NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL)

static struct net_mgmt_event_callback mgmt_cb;

static void event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			  struct net_if *iface)
{
	LOG_INF("mgmt_event = %d", mgmt_event);
	if ((mgmt_event & EVENT_MASK) != mgmt_event) {
		return;
	}
	switch (mgmt_event) {
	case NET_EVENT_IPV6_ADDR_ADD:
		LOG_INF("NET_EVENT_IPV6_ADDR_ADD");
		break;
	case NET_EVENT_IPV6_ADDR_DEL:
		LOG_INF("NET_EVENT_IPV6_ADDR_DEL");
		break;
	case NET_EVENT_IPV6_MADDR_ADD:
		LOG_INF("NET_EVENT_IPV6_MADDR_ADD");
		break;
	case NET_EVENT_IPV6_MADDR_DEL:
		LOG_INF("NET_EVENT_IPV6_MADDR_DEL");
		break;
	case NET_EVENT_IPV6_PREFIX_ADD:
		LOG_INF("NET_EVENT_IPV6_PREFIX_ADD");
		break;
	case NET_EVENT_IPV6_PREFIX_DEL:
		LOG_INF("NET_EVENT_IPV6_PREFIX_DEL");
		break;
	case NET_EVENT_IPV6_MCAST_JOIN:
		LOG_INF("NET_EVENT_IPV6_MCAST_JOIN");
		break;
	case NET_EVENT_IPV6_MCAST_LEAVE:
		LOG_INF("NET_EVENT_IPV6_MCAST_LEAVE");
		break;
	case NET_EVENT_IPV6_ROUTER_ADD:
		LOG_INF("NET_EVENT_IPV6_ROUTER_ADD");
		break;
	case NET_EVENT_IPV6_ROUTER_DEL:
		LOG_INF("NET_EVENT_IPV6_ROUTER_DEL");
		break;
	case NET_EVENT_IPV6_ROUTE_ADD:
		LOG_INF("NET_EVENT_IPV6_ROUTE_ADD");
		break;
	case NET_EVENT_IPV6_ROUTE_DEL:
		LOG_INF("NET_EVENT_IPV6_ROUTE_DEL");
		break;
	case NET_EVENT_IPV6_DAD_SUCCEED:
		LOG_INF("NET_EVENT_IPV6_DAD_SUCCEED");
		break;
	case NET_EVENT_IPV6_DAD_FAILED:
		LOG_INF("NET_EVENT_IPV6_DAD_FAILED");
		break;
	case NET_EVENT_IPV6_NBR_ADD:
		LOG_INF("NET_EVENT_IPV6_NBR_ADD");
		break;
	case NET_EVENT_IPV6_NBR_DEL:
		LOG_INF("NET_EVENT_IPV6_NBR_DEL");
		break;
	default:
		break;
	}

	if (mgmt_event == NET_EVENT_IPV6_ADDR_ADD) {
		LOG_INF("Network connected");

		if (smp_udp_open() < 0) {
			LOG_ERR("could not open smp udp");
		}

		return;
	}

	if (mgmt_event == NET_EVENT_IPV6_ADDR_DEL) {
		LOG_INF("Network disconnected");
		smp_udp_close();
		return;
	}
}

void start_smp_udp(void)
{
	LOG_INF("%d,%s", __LINE__, __FUNCTION__);
	net_mgmt_init_event_callback(&mgmt_cb, event_handler, EVENT_MASK);
	net_mgmt_add_event_callback(&mgmt_cb);
	net_conn_mgr_resend_status();
}

void smp_start(void)
{
	smp_udp_close();
	
	if (smp_udp_open() < 0) {
		LOG_ERR("could not open smp udp");
	}
	
	LOG_INF("SMP udp start ... ");

	return;
}

void smp_stop(void)
{
	smp_udp_close();

	LOG_INF("SMP udp stop ... ");

	return;
}