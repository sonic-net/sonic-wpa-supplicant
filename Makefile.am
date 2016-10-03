bin_SCRIPTS = scripts/bin/*

pyutilsdir=$(libdir)/sonic
pyutils_SCRIPTS = scripts/lib/python/*.py

include_HEADERS=sonic/nas_qos_switch.h sonic/nas_qos_queue.h sonic/nas_qos_buffer_profile.h sonic/nas_qos_common.h sonic/nas_qos_cps_buffer_pool.h sonic/nas_qos_cps_priority_group.h sonic/nas_qos_policer.h sonic/nas_qos_cps_port_ingress.h sonic/nas_qos_cps_queue.h sonic/nas_qos_cps_scheduler_group.h sonic/nas_qos_buffer_pool.h sonic/nas_qos_switch_list.h sonic/nas_qos_cps_buffer_profile.h sonic/nas_qos_cps_port_egress.h sonic/nas_qos_cps_policer.h sonic/nas_qos_map.h sonic/nas_qos_scheduler.h sonic/nas_qos_cps_map.h sonic/nas_qos_priority_group.h sonic/nas_qos_map_util.h sonic/nas_qos_cps_wred.h sonic/nas_qos_cps.h sonic/nas_qos_port_ingress.h sonic/nas_qos_map_entry.h sonic/nas_qos_init.h sonic/nas_qos_port_egress.h sonic/nas_qos_cps_scheduler.h sonic/nas_qos_wred.h sonic/nas_qos_scheduler_group.h
lib_LTLIBRARIES=libsonic_nas_qos.la

libsonic_nas_qos_la_SOURCES=src/nas_qos_cps_map_entry.cpp src/nas_qos_policer.cpp src/nas_qos_buffer_profile.cpp src/nas_qos_map_util.cpp src/nas_qos_switch.cpp src/nas_qos_cps_scheduler.cpp src/nas_qos_map.cpp src/nas_qos_port_ingress.cpp src/nas_qos_cps_buffer_profile.cpp src/nas_qos_scheduler_group.cpp src/nas_qos_switch_list.cpp src/nas_qos_cps_buffer_pool.cpp src/nas_qos_map_entry.cpp src/nas_qos_cps_port_egress.cpp src/nas_qos_cps_wred.cpp src/nas_qos_cps_queue.cpp src/nas_qos_queue.cpp src/nas_qos_init.cpp src/nas_qos_cps_port_ingress.cpp src/nas_qos_wred.cpp src/nas_qos_cps_policer.cpp src/nas_qos_buffer_pool.cpp src/nas_qos_cps_map.cpp src/nas_qos_cps_priority_group.cpp src/nas_qos_port_egress.cpp src/nas_qos_cps.cpp src/nas_qos_cps_scheduler_group.cpp src/nas_qos_scheduler.cpp src/nas_qos_priority_group.cpp

libsonic_nas_qos_la_CPPFLAGS=-D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/sonic -I$(includedir)/sonic
libsonic_nas_qos_la_CXXFLAGS=-std=c++11
libsonic_nas_qos_la_LDFLAGS=-shared -version-info 1:1:0
libsonic_nas_qos_la_LIBADD=-lsonic_logging -lsonic_nas_ndi -lsonic_nas_common -lsonic_object_library -lsonic_common

systemdconfdir=/lib/systemd/system
systemdconf_DATA = scripts/init/*.service
