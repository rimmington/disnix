/*
 * Disnix - A Nix-based distributed service deployment tool
 * Copyright (C) 2008-2017  Sander van der Burg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __DISNIX_CLIENT_INTERFACE_H
#define __DISNIX_CLIENT_INTERFACE_H

#include <unistd.h>
#include <glib.h>
#include <procreact_future.h>

/**
 * Invokes the activate operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param container Name of the container in which the component is deployed
 * @param type Type Type of the service
 * @param arguments String vector with activation arguments in the form key=value
 * @param arguments_size Size of the arguments string vector
 * @param service Service to activate
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_activate(gchar *interface, gchar *target, gchar *container, gchar *type, gchar **arguments, const unsigned int arguments_size, gchar *service);

/**
 * Invokes the deactivate operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param container Name of the container in which the component is deployed
 * @param type Type Type of the service
 * @param arguments String vector with activation arguments in the form key=value
 * @param arguments_size Size of the arguments string vector
 * @param service Service to deactivate
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_deactivate(gchar *interface, gchar *target, gchar *container, gchar *type, gchar **arguments, const unsigned int arguments_size, gchar *service);

/**
 * Invokes the lock operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param profile Identifier of the distributed profile
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_lock(gchar *interface, gchar *target, gchar *profile);

/**
 * Invokes the unlock operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param profile Identifier of the distributed profile
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_unlock(gchar *interface, gchar *target, gchar *profile);

/**
 * Invokes the snapshot operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param container Name of the container in which the component is deployed
 * @param type Type Type of the service
 * @param arguments String vector with activation arguments in the form key=value
 * @param arguments_size Size of the arguments string vector
 * @param service Service to activate
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_snapshot(gchar *interface, gchar *target, gchar *container, gchar *type, gchar **arguments, const unsigned int arguments_size, gchar *service);

/**
 * Invokes the restore operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param container Name of the container in which the component is deployed
 * @param type Type Type of the service
 * @param arguments String vector with activation arguments in the form key=value
 * @param arguments_size Size of the arguments string vector
 * @param service Service to deactivate
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_restore(gchar *interface, gchar *target, gchar *container, gchar *type, gchar **arguments, const unsigned int arguments_size, gchar *service);

/**
 * Invokes the delete state operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param container Name of the container in which the component is deployed
 * @param type Type Type of the service
 * @param arguments String vector with activation arguments in the form key=value
 * @param arguments_size Size of the arguments string vector
 * @param service Service to activate
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_delete_state(gchar *interface, gchar *target, gchar *container, gchar *type, gchar **arguments, const unsigned int arguments_size, gchar *service);

/**
 * Invokes the collect garbage operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param delete_old Indicates whether old profile generations must be removed
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_collect_garbage(gchar *interface, gchar *target, const gboolean delete_old);

/**
 * Invokes the set operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param profile Identifier of the distributed profile
 * @param component Component which becomes the contents of the profile
 */
pid_t exec_set(gchar *interface, gchar *target, gchar *profile, gchar *component);

/**
 * Invokes the the query installed operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param profile Identifier of the distributed profile
 * @return Future struct of the client interface process performing the operation
 */
ProcReact_Future exec_query_installed(gchar *interface, gchar *target, gchar *profile);

/**
 * Invokes the copy closure process to copy a closure from a machine
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param paths Nix store paths to copy (including all intra-dependencies)
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_copy_closure_from(gchar *interface, gchar *target, gchar **paths);

/**
 * Invokes the copy closure process to copy a closure to a machine
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param paths Nix store paths to copy (including all intra-dependencies)
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_copy_closure_to(gchar *interface, gchar *target, gchar **paths);

/**
 * Invokes the copy snapshots process to copy snapshots from a machine
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param container Name of the container in which the component is deployed
 * @param component Name of the component of which state snapshots should be copied
 * @param all Indicates whether all generations of snapshots must be transferred
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_copy_snapshots_from(gchar *interface, gchar *target, gchar *container, gchar *component, gboolean all);

/**
 * Invokes the copy snapshots process to copy snapshots to a machine
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param container Name of the container in which the component is deployed
 * @param component Name of the component of which state snapshots should be copied
 * @param all Indicates whether all generations of snapshots must be transferred
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_copy_snapshots_to(gchar *interface, gchar *target, gchar *container, gchar *component, gboolean all);

/**
 * Invokes the Dysnomia snapshot garbage collect operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param keep Number of snapshot generations to keep
 * @param container Name of the container to filter on, or NULL to consult all containers
 * @param component Name of the component to filter on, or NULL to consult all components
 * @return PID of the client interface process performing the operation, or -1 in case of a failure
 */
pid_t exec_clean_snapshots(gchar *interface, gchar *target, int keep, char *container, char *component);

/**
 * Invokes the realise operation through a Disnix client interface
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param derivation Derivation to build
 * @return Future struct of the client interface process performing the operation
 */
ProcReact_Future exec_realise(gchar *interface, gchar *target, gchar *derivation);

/**
 * Captures the configuration from the Dysnomia container configuration files
 * and generates a Nix expression from it.
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @return Future struct of the client interface process performing the operation
 */
ProcReact_Future exec_capture_config(gchar *interface, gchar *target);

/**
 * Queries the requisites of a given derivation
 *
 * @param interface Path to the interface executable
 * @param target Target Address of the remote interface
 * @param derivation Derivation to query the requisities from
 * @return Future struct of the client interface process performing the operation
 */
ProcReact_Future exec_query_requisites(gchar *interface, gchar *target, gchar *derivation);

/**
 * Invokes the true command for testing purposes.
 */
pid_t exec_true(void);

#endif
