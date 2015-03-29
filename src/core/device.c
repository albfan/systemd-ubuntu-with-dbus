/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <errno.h>
#include <sys/epoll.h>
#include <libudev.h>

#include "strv.h"
#include "log.h"
#include "unit-name.h"
#include "dbus-device.h"
#include "def.h"
#include "path-util.h"
#include "udev-util.h"
#include "unit.h"
#include "swap.h"
#include "device.h"

static const UnitActiveState state_translation_table[_DEVICE_STATE_MAX] = {
        [DEVICE_DEAD] = UNIT_INACTIVE,
        [DEVICE_TENTATIVE] = UNIT_ACTIVATING,
        [DEVICE_PLUGGED] = UNIT_ACTIVE,
};

static int device_dispatch_io(sd_event_source *source, int fd, uint32_t revents, void *userdata);

static void device_unset_sysfs(Device *d) {
        Hashmap *devices;
        Device *first;

        assert(d);

        if (!d->sysfs)
                return;

        /* Remove this unit from the chain of devices which share the
         * same sysfs path. */
        devices = UNIT(d)->manager->devices_by_sysfs;
        first = hashmap_get(devices, d->sysfs);
        LIST_REMOVE(same_sysfs, first, d);

        if (first)
                hashmap_remove_and_replace(devices, d->sysfs, first->sysfs, first);
        else
                hashmap_remove(devices, d->sysfs);

        free(d->sysfs);
        d->sysfs = NULL;
}

static int device_set_sysfs(Device *d, const char *sysfs) {
        Device *first;
        char *copy;
        int r;

        assert(d);

        if (streq_ptr(d->sysfs, sysfs))
                return 0;

        r = hashmap_ensure_allocated(&UNIT(d)->manager->devices_by_sysfs, &string_hash_ops);
        if (r < 0)
                return r;

        copy = strdup(sysfs);
        if (!copy)
                return -ENOMEM;

        device_unset_sysfs(d);

        first = hashmap_get(UNIT(d)->manager->devices_by_sysfs, sysfs);
        LIST_PREPEND(same_sysfs, first, d);

        r = hashmap_replace(UNIT(d)->manager->devices_by_sysfs, copy, first);
        if (r < 0) {
                LIST_REMOVE(same_sysfs, first, d);
                free(copy);
                return r;
        }

        d->sysfs = copy;

        return 0;
}

static void device_init(Unit *u) {
        Device *d = DEVICE(u);

        assert(d);
        assert(UNIT(d)->load_state == UNIT_STUB);

        /* In contrast to all other unit types we timeout jobs waiting
         * for devices by default. This is because they otherwise wait
         * indefinitely for plugged in devices, something which cannot
         * happen for the other units since their operations time out
         * anyway. */
        u->job_timeout = u->manager->default_timeout_start_usec;

        u->ignore_on_isolate = true;
        u->ignore_on_snapshot = true;
}

static void device_done(Unit *u) {
        Device *d = DEVICE(u);

        assert(d);

        device_unset_sysfs(d);
}

static void device_set_state(Device *d, DeviceState state) {
        DeviceState old_state;
        assert(d);

        old_state = d->state;
        d->state = state;

        if (state != old_state)
                log_unit_debug(UNIT(d)->id,
                               "%s changed %s -> %s", UNIT(d)->id,
                               device_state_to_string(old_state),
                               device_state_to_string(state));

        unit_notify(UNIT(d), state_translation_table[old_state], state_translation_table[state], true);
}

static int device_coldplug(Unit *u, Hashmap *deferred_work) {
        Device *d = DEVICE(u);

        assert(d);
        assert(d->state == DEVICE_DEAD);

        if (d->found & DEVICE_FOUND_UDEV)
                /* If udev says the device is around, it's around */
                device_set_state(d, DEVICE_PLUGGED);
        else if (d->found != DEVICE_NOT_FOUND)
                /* If a device is found in /proc/self/mountinfo or
                 * /proc/swaps, it's "tentatively" around. */
                device_set_state(d, DEVICE_TENTATIVE);

        return 0;
}

static void device_dump(Unit *u, FILE *f, const char *prefix) {
        Device *d = DEVICE(u);

        assert(d);

        fprintf(f,
                "%sDevice State: %s\n"
                "%sSysfs Path: %s\n",
                prefix, device_state_to_string(d->state),
                prefix, strna(d->sysfs));
}

_pure_ static UnitActiveState device_active_state(Unit *u) {
        assert(u);

        return state_translation_table[DEVICE(u)->state];
}

_pure_ static const char *device_sub_state_to_string(Unit *u) {
        assert(u);

        return device_state_to_string(DEVICE(u)->state);
}

static int device_update_description(Unit *u, struct udev_device *dev, const char *path) {
        const char *model;
        int r;

        assert(u);
        assert(dev);
        assert(path);

        model = udev_device_get_property_value(dev, "ID_MODEL_FROM_DATABASE");
        if (!model)
                model = udev_device_get_property_value(dev, "ID_MODEL");

        if (model) {
                const char *label;

                /* Try to concatenate the device model string with a label, if there is one */
                label = udev_device_get_property_value(dev, "ID_FS_LABEL");
                if (!label)
                        label = udev_device_get_property_value(dev, "ID_PART_ENTRY_NAME");
                if (!label)
                        label = udev_device_get_property_value(dev, "ID_PART_ENTRY_NUMBER");

                if (label) {
                        _cleanup_free_ char *j;

                        j = strjoin(model, " ", label, NULL);
                        if (j)
                                r = unit_set_description(u, j);
                        else
                                r = -ENOMEM;
                } else
                        r = unit_set_description(u, model);
        } else
                r = unit_set_description(u, path);

        if (r < 0)
                log_unit_error_errno(u->id, r, "Failed to set device description: %m");

        return r;
}

static int device_add_udev_wants(Unit *u, struct udev_device *dev) {
        const char *wants;
        const char *word, *state;
        size_t l;
        int r;
        const char *property;

        assert(u);
        assert(dev);

        property = u->manager->running_as == SYSTEMD_USER ? "SYSTEMD_USER_WANTS" : "SYSTEMD_WANTS";
        wants = udev_device_get_property_value(dev, property);
        if (!wants)
                return 0;

        FOREACH_WORD_QUOTED(word, l, wants, state) {
                _cleanup_free_ char *n = NULL;
                char e[l+1];

                memcpy(e, word, l);
                e[l] = 0;

                n = unit_name_mangle(e, MANGLE_NOGLOB);
                if (!n)
                        return log_oom();

                r = unit_add_dependency_by_name(u, UNIT_WANTS, n, NULL, true);
                if (r < 0)
                        return log_unit_error_errno(u->id, r, "Failed to add wants dependency: %m");
        }
        if (!isempty(state))
                log_unit_warning(u->id, "Property %s on %s has trailing garbage, ignoring.", property, strna(udev_device_get_syspath(dev)));

        return 0;
}

static int device_setup_unit(Manager *m, struct udev_device *dev, const char *path, bool main) {
        _cleanup_free_ char *e = NULL;
        const char *sysfs;
        Unit *u = NULL;
        bool delete;
        int r;

        assert(m);
        assert(dev);
        assert(path);

        sysfs = udev_device_get_syspath(dev);
        if (!sysfs)
                return 0;

        e = unit_name_from_path(path, ".device");
        if (!e)
                return log_oom();

        u = manager_get_unit(m, e);

        if (u &&
            DEVICE(u)->sysfs &&
            !path_equal(DEVICE(u)->sysfs, sysfs)) {
                log_unit_error(u->id, "Device %s appeared twice with different sysfs paths %s and %s", e, DEVICE(u)->sysfs, sysfs);
                return -EEXIST;
        }

        if (!u) {
                delete = true;

                u = unit_new(m, sizeof(Device));
                if (!u)
                        return log_oom();

                r = unit_add_name(u, e);
                if (r < 0)
                        goto fail;

                unit_add_to_load_queue(u);
        } else
                delete = false;

        /* If this was created via some dependency and has not
         * actually been seen yet ->sysfs will not be
         * initialized. Hence initialize it if necessary. */

        r = device_set_sysfs(DEVICE(u), sysfs);
        if (r < 0)
                goto fail;

        (void) device_update_description(u, dev, path);

        /* The additional systemd udev properties we only interpret
         * for the main object */
        if (main)
                (void) device_add_udev_wants(u, dev);

        /* Note that this won't dispatch the load queue, the caller
         * has to do that if needed and appropriate */

        unit_add_to_dbus_queue(u);
        return 0;

fail:
        log_unit_warning_errno(u->id, r, "Failed to set up device unit: %m");

        if (delete && u)
                unit_free(u);

        return r;
}

static int device_process_new(Manager *m, struct udev_device *dev) {
        const char *sysfs, *dn, *alias;
        struct udev_list_entry *item = NULL, *first = NULL;
        int r;

        assert(m);

        sysfs = udev_device_get_syspath(dev);
        if (!sysfs)
                return 0;

        /* Add the main unit named after the sysfs path */
        r = device_setup_unit(m, dev, sysfs, true);
        if (r < 0)
                return r;

        /* Add an additional unit for the device node */
        dn = udev_device_get_devnode(dev);
        if (dn)
                (void) device_setup_unit(m, dev, dn, false);

        /* Add additional units for all symlinks */
        first = udev_device_get_devlinks_list_entry(dev);
        udev_list_entry_foreach(item, first) {
                const char *p;
                struct stat st;

                /* Don't bother with the /dev/block links */
                p = udev_list_entry_get_name(item);

                if (path_startswith(p, "/dev/block/") ||
                    path_startswith(p, "/dev/char/"))
                        continue;

                /* Verify that the symlink in the FS actually belongs
                 * to this device. This is useful to deal with
                 * conflicting devices, e.g. when two disks want the
                 * same /dev/disk/by-label/xxx link because they have
                 * the same label. We want to make sure that the same
                 * device that won the symlink wins in systemd, so we
                 * check the device node major/minor */
                if (stat(p, &st) >= 0)
                        if ((!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode)) ||
                            st.st_rdev != udev_device_get_devnum(dev))
                                continue;

                (void) device_setup_unit(m, dev, p, false);
        }

        /* Add additional units for all explicitly configured
         * aliases */
        alias = udev_device_get_property_value(dev, "SYSTEMD_ALIAS");
        if (alias) {
                const char *word, *state;
                size_t l;

                FOREACH_WORD_QUOTED(word, l, alias, state) {
                        char e[l+1];

                        memcpy(e, word, l);
                        e[l] = 0;

                        if (path_is_absolute(e))
                                (void) device_setup_unit(m, dev, e, false);
                        else
                                log_warning("SYSTEMD_ALIAS for %s is not an absolute path, ignoring: %s", sysfs, e);
                }
                if (!isempty(state))
                        log_warning("SYSTEMD_ALIAS for %s has trailing garbage, ignoring.", sysfs);
        }

        return 0;
}

static void device_update_found_one(Device *d, bool add, DeviceFound found, bool now) {
        DeviceFound n;

        assert(d);

        n = add ? (d->found | found) : (d->found & ~found);
        if (n == d->found)
                return;

        d->found = n;

        if (now) {
                if (d->found & DEVICE_FOUND_UDEV)
                        device_set_state(d, DEVICE_PLUGGED);
                else if (add && d->found != DEVICE_NOT_FOUND)
                        device_set_state(d, DEVICE_TENTATIVE);
                else
                        device_set_state(d, DEVICE_DEAD);
        }
}

static int device_update_found_by_sysfs(Manager *m, const char *sysfs, bool add, DeviceFound found, bool now) {
        Device *d, *l;

        assert(m);
        assert(sysfs);

        if (found == DEVICE_NOT_FOUND)
                return 0;

        l = hashmap_get(m->devices_by_sysfs, sysfs);
        LIST_FOREACH(same_sysfs, d, l)
                device_update_found_one(d, add, found, now);

        return 0;
}

static int device_update_found_by_name(Manager *m, const char *path, bool add, DeviceFound found, bool now) {
        _cleanup_free_ char *e = NULL;
        Unit *u;

        assert(m);
        assert(path);

        if (found == DEVICE_NOT_FOUND)
                return 0;

        e = unit_name_from_path(path, ".device");
        if (!e)
                return log_oom();

        u = manager_get_unit(m, e);
        if (!u)
                return 0;

        device_update_found_one(DEVICE(u), add, found, now);
        return 0;
}

static bool device_is_ready(struct udev_device *dev) {
        const char *ready;

        assert(dev);

        ready = udev_device_get_property_value(dev, "SYSTEMD_READY");
        if (!ready)
                return true;

        return parse_boolean(ready) != 0;
}

static Unit *device_following(Unit *u) {
        Device *d = DEVICE(u);
        Device *other, *first = NULL;

        assert(d);

        if (startswith(u->id, "sys-"))
                return NULL;

        /* Make everybody follow the unit that's named after the sysfs path */
        for (other = d->same_sysfs_next; other; other = other->same_sysfs_next)
                if (startswith(UNIT(other)->id, "sys-"))
                        return UNIT(other);

        for (other = d->same_sysfs_prev; other; other = other->same_sysfs_prev) {
                if (startswith(UNIT(other)->id, "sys-"))
                        return UNIT(other);

                first = other;
        }

        return UNIT(first);
}

static int device_following_set(Unit *u, Set **_set) {
        Device *d = DEVICE(u), *other;
        Set *set;
        int r;

        assert(d);
        assert(_set);

        if (LIST_JUST_US(same_sysfs, d)) {
                *_set = NULL;
                return 0;
        }

        set = set_new(NULL);
        if (!set)
                return -ENOMEM;

        LIST_FOREACH_AFTER(same_sysfs, other, d) {
                r = set_put(set, other);
                if (r < 0)
                        goto fail;
        }

        LIST_FOREACH_BEFORE(same_sysfs, other, d) {
                r = set_put(set, other);
                if (r < 0)
                        goto fail;
        }

        *_set = set;
        return 1;

fail:
        set_free(set);
        return r;
}

static void device_shutdown(Manager *m) {
        assert(m);

        m->udev_event_source = sd_event_source_unref(m->udev_event_source);

        if (m->udev_monitor) {
                udev_monitor_unref(m->udev_monitor);
                m->udev_monitor = NULL;
        }

        hashmap_free(m->devices_by_sysfs);
        m->devices_by_sysfs = NULL;
}

static int device_enumerate(Manager *m) {
        _cleanup_udev_enumerate_unref_ struct udev_enumerate *e = NULL;
        struct udev_list_entry *item = NULL, *first = NULL;
        int r;

        assert(m);

        if (!m->udev_monitor) {
                m->udev_monitor = udev_monitor_new_from_netlink(m->udev, "udev");
                if (!m->udev_monitor) {
                        r = -ENOMEM;
                        goto fail;
                }

                /* This will fail if we are unprivileged, but that
                 * should not matter much, as user instances won't run
                 * during boot. */
                udev_monitor_set_receive_buffer_size(m->udev_monitor, 128*1024*1024);

                r = udev_monitor_filter_add_match_tag(m->udev_monitor, "systemd");
                if (r < 0)
                        goto fail;

                r = udev_monitor_enable_receiving(m->udev_monitor);
                if (r < 0)
                        goto fail;

                r = sd_event_add_io(m->event, &m->udev_event_source, udev_monitor_get_fd(m->udev_monitor), EPOLLIN, device_dispatch_io, m);
                if (r < 0)
                        goto fail;
        }

        e = udev_enumerate_new(m->udev);
        if (!e) {
                r = -ENOMEM;
                goto fail;
        }

        r = udev_enumerate_add_match_tag(e, "systemd");
        if (r < 0)
                goto fail;

        r = udev_enumerate_add_match_is_initialized(e);
        if (r < 0)
                goto fail;

        r = udev_enumerate_scan_devices(e);
        if (r < 0)
                goto fail;

        first = udev_enumerate_get_list_entry(e);
        udev_list_entry_foreach(item, first) {
                _cleanup_udev_device_unref_ struct udev_device *dev = NULL;
                const char *sysfs;

                sysfs = udev_list_entry_get_name(item);

                dev = udev_device_new_from_syspath(m->udev, sysfs);
                if (!dev) {
                        log_oom();
                        continue;
                }

                if (!device_is_ready(dev))
                        continue;

                (void) device_process_new(m, dev);

                device_update_found_by_sysfs(m, sysfs, true, DEVICE_FOUND_UDEV, false);
        }

        return 0;

fail:
        log_error_errno(r, "Failed to enumerate devices: %m");

        device_shutdown(m);
        return r;
}

static int device_dispatch_io(sd_event_source *source, int fd, uint32_t revents, void *userdata) {
        _cleanup_udev_device_unref_ struct udev_device *dev = NULL;
        Manager *m = userdata;
        const char *action, *sysfs;
        int r;

        assert(m);

        if (revents != EPOLLIN) {
                static RATELIMIT_DEFINE(limit, 10*USEC_PER_SEC, 5);

                if (!ratelimit_test(&limit))
                        log_error_errno(errno, "Failed to get udev event: %m");
                if (!(revents & EPOLLIN))
                        return 0;
        }

        /*
         * libudev might filter-out devices which pass the bloom
         * filter, so getting NULL here is not necessarily an error.
         */
        dev = udev_monitor_receive_device(m->udev_monitor);
        if (!dev)
                return 0;

        sysfs = udev_device_get_syspath(dev);
        if (!sysfs) {
                log_error("Failed to get udev sys path.");
                return 0;
        }

        action = udev_device_get_action(dev);
        if (!action) {
                log_error("Failed to get udev action string.");
                return 0;
        }

        if (streq(action, "remove"))  {
                r = swap_process_device_remove(m, dev);
                if (r < 0)
                        log_error_errno(r, "Failed to process swap device remove event: %m");

                /* If we get notified that a device was removed by
                 * udev, then it's completely gone, hence unset all
                 * found bits */
                device_update_found_by_sysfs(m, sysfs, false, DEVICE_FOUND_UDEV|DEVICE_FOUND_MOUNT|DEVICE_FOUND_SWAP, true);

        } else if (device_is_ready(dev)) {

                (void) device_process_new(m, dev);

                r = swap_process_device_new(m, dev);
                if (r < 0)
                        log_error_errno(r, "Failed to process swap device new event: %m");

                manager_dispatch_load_queue(m);

                /* The device is found now, set the udev found bit */
                device_update_found_by_sysfs(m, sysfs, true, DEVICE_FOUND_UDEV, true);

        } else {
                /* The device is nominally around, but not ready for
                 * us. Hence unset the udev bit, but leave the rest
                 * around. */

                device_update_found_by_sysfs(m, sysfs, false, DEVICE_FOUND_UDEV, true);
        }

        return 0;
}

static bool device_supported(Manager *m) {
        static int read_only = -1;
        assert(m);

        /* If /sys is read-only we don't support device units, and any
         * attempts to start one should fail immediately. */

        if (read_only < 0)
                read_only = path_is_read_only_fs("/sys");

        return read_only <= 0;
}

int device_found_node(Manager *m, const char *node, bool add, DeviceFound found, bool now) {
        _cleanup_udev_device_unref_ struct udev_device *dev = NULL;
        struct stat st;

        assert(m);
        assert(node);

        /* This is called whenever we find a device referenced in
         * /proc/swaps or /proc/self/mounts. Such a device might be
         * mounted/enabled at a time where udev has not finished
         * probing it yet, and we thus haven't learned about it
         * yet. In this case we will set the device unit to
         * "tentative" state. */

        if (add) {
                if (!path_startswith(node, "/dev"))
                        return 0;

                if (stat(node, &st) < 0) {
                        if (errno == ENOENT)
                                return 0;

                        return log_error_errno(errno, "Failed to stat device node file %s: %m", node);
                }

                if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode))
                        return 0;

                dev = udev_device_new_from_devnum(m->udev, S_ISBLK(st.st_mode) ? 'b' : 'c', st.st_rdev);
                if (!dev) {
                        if (errno == ENOENT)
                                return 0;

                        return log_oom();
                }

                /* If the device is known in the kernel and newly
                 * appeared, then we'll create a device unit for it,
                 * under the name referenced in /proc/swaps or
                 * /proc/self/mountinfo. */

                (void) device_setup_unit(m, dev, node, false);
        }

        /* Update the device unit's state, should it exist */
        return device_update_found_by_name(m, node, add, found, now);
}

static const char* const device_state_table[_DEVICE_STATE_MAX] = {
        [DEVICE_DEAD] = "dead",
        [DEVICE_TENTATIVE] = "tentative",
        [DEVICE_PLUGGED] = "plugged",
};

DEFINE_STRING_TABLE_LOOKUP(device_state, DeviceState);

const UnitVTable device_vtable = {
        .object_size = sizeof(Device),
        .sections =
                "Unit\0"
                "Device\0"
                "Install\0",

        .no_instances = true,

        .init = device_init,
        .done = device_done,
        .load = unit_load_fragment_and_dropin_optional,

        .coldplug = device_coldplug,

        .dump = device_dump,

        .active_state = device_active_state,
        .sub_state_to_string = device_sub_state_to_string,

        .bus_interface = "org.freedesktop.systemd1.Device",
        .bus_vtable = bus_device_vtable,

        .following = device_following,
        .following_set = device_following_set,

        .enumerate = device_enumerate,
        .shutdown = device_shutdown,
        .supported = device_supported,

        .status_message_formats = {
                .starting_stopping = {
                        [0] = "Expecting device %s...",
                },
                .finished_start_job = {
                        [JOB_DONE]       = "Found device %s.",
                        [JOB_TIMEOUT]    = "Timed out waiting for device %s.",
                },
        },
};
