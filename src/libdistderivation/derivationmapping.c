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

#include "derivationmapping.h"
#include <xmlutil.h>

GPtrArray *create_derivation_array(const gchar *distributed_derivation_file)
{
    /* Declarations */
    xmlDocPtr doc;
    xmlNodePtr node_root;
    xmlXPathObjectPtr result;
    GPtrArray *derivation_array = NULL;
    
    /* Parse the XML document */
    
    if((doc = xmlParseFile(distributed_derivation_file)) == NULL)
    {
	g_printerr("Error with parsing the distributed derivation XML file!\n");
	xmlCleanupParser();
	return NULL;
    }
    
    /* Retrieve root element */
    node_root = xmlDocGetRootElement(doc);
    
    if(node_root == NULL)
    {
        g_printerr("The distributed derivation XML file is empty!\n");
	xmlFreeDoc(doc);
	xmlCleanupParser();
	return NULL;
    }

    /* Query the mapping elements */
    result = executeXPathQuery(doc, "/distributedderivation/build/mapping");
    
    /* Iterate over all the mapping elements and add them to the array */
    
    if(result)
    {
	xmlNodeSetPtr nodeset = result->nodesetval;
	unsigned int i;
	
	/* Create a derivation array */
        derivation_array = g_ptr_array_new();
	
	/* Iterate over all the mapping elements */
	for(i = 0; i < nodeset->nodeNr; i++)
        {
	    xmlNodePtr mapping_children = nodeset->nodeTab[i]->children;
	    DerivationItem *item = (DerivationItem*)g_malloc(sizeof(DerivationItem));
	    gchar *derivation = NULL, *target = NULL;
	    
	    /* Iterate over all the mapping item children (derivation and target elements) */
	    
	    while(mapping_children != NULL)
	    {
		if(xmlStrcmp(mapping_children->name, (xmlChar*) "derivation") == 0)
		    derivation = duplicate_node_text(mapping_children);
		else if(xmlStrcmp(mapping_children->name, (xmlChar*) "target") == 0)
		    target = duplicate_node_text(mapping_children);
		
		mapping_children = mapping_children->next;
	    }
	    
	    /* Added the mapping to the array */
	    item->derivation = derivation;
	    item->target = target;
	    item->result = NULL;
	    
	    if(item->derivation == NULL || item->target == NULL)
	    {
	        /* Check if all mandatory properties have been provided */
	        g_printerr("A mandatory property seems to be missing. Have you provided a correct\n");
	        g_printerr("distributed derivation file?\n");
	        delete_derivation_array(derivation_array);
	        derivation_array = NULL;
	        break;
	    }
	    else
	        g_ptr_array_add(derivation_array, item); /* Add item to the array */
        }
        
        xmlXPathFreeObject(result);
    }
    
    /* Cleanup */
    xmlFreeDoc(doc);
    xmlCleanupParser();
    
    /* Return the derivation array */
    return derivation_array;
}

void delete_derivation_array(GPtrArray *derivation_array)
{
    if(derivation_array != NULL)
    {
        unsigned int i;
    
        for(i = 0; i < derivation_array->len; i++)
        {
            DerivationItem *item = g_ptr_array_index(derivation_array, i);
            free(item->derivation);
            g_free(item->target);
            g_strfreev(item->result);
            g_free(item);
        }
    
        g_ptr_array_free(derivation_array, TRUE);
    }
}

static int has_next_derivation_item(void *data)
{
    DerivationIteratorData *derivation_iterator_data = (DerivationIteratorData*)data;
    return has_next_iteration_process(&derivation_iterator_data->model_iterator_data);
}

static pid_t next_derivation_process(void *data)
{
    /* Declarations */
    DerivationIteratorData *derivation_iterator_data = (DerivationIteratorData*)data;
    
    /* Retrieve derivation item, interface pair */
    DerivationItem *item = g_ptr_array_index(derivation_iterator_data->derivation_array, derivation_iterator_data->model_iterator_data.index);
    Interface *interface = find_interface(derivation_iterator_data->interface_array, item->target);
    
    /* Invoke the next derivation item operation process */
    pid_t pid = derivation_iterator_data->map_derivation_item_function.pid(derivation_iterator_data->data, item, interface);
    
    /* Increase the iterator index and update the pid table */
    next_iteration_process(&derivation_iterator_data->model_iterator_data, pid, item);
    
    /* Return the pid of the invoked process */
    return pid;
}

static void complete_derivation_process(void *data, pid_t pid, ProcReact_Status status, int result)
{
    DerivationIteratorData *derivation_iterator_data = (DerivationIteratorData*)data;
    
    /* Retrieve the completed item */
    DerivationItem *item = complete_iteration_process(&derivation_iterator_data->model_iterator_data, pid, status, result);
    
    /* Invoke callback that handles the completion of derivation item mapping */
    derivation_iterator_data->complete_derivation_item_mapping_function.pid(derivation_iterator_data->data, item, status, result);
}

static DerivationIteratorData *create_common_iterator(const GPtrArray *derivation_array, const GPtrArray *interface_array, void *data)
{
    DerivationIteratorData *derivation_iterator_data = (DerivationIteratorData*)g_malloc(sizeof(DerivationIteratorData));
    
    init_model_iterator_data(&derivation_iterator_data->model_iterator_data, derivation_array->len);
    derivation_iterator_data->derivation_array = derivation_array;
    derivation_iterator_data->interface_array = interface_array;
    derivation_iterator_data->data = data;
    
    return derivation_iterator_data;
}

ProcReact_PidIterator create_derivation_pid_iterator(const GPtrArray *derivation_array, const GPtrArray *interface_array, map_derivation_item_pid_function map_derivation_item, complete_derivation_item_mapping_pid_function complete_derivation_item_mapping, void *data)
{
    DerivationIteratorData *derivation_iterator_data = create_common_iterator(derivation_array, interface_array, data);
    derivation_iterator_data->map_derivation_item_function.pid = map_derivation_item;
    derivation_iterator_data->complete_derivation_item_mapping_function.pid = complete_derivation_item_mapping;
    
    return procreact_initialize_pid_iterator(has_next_derivation_item, next_derivation_process, procreact_retrieve_boolean, complete_derivation_process, derivation_iterator_data);
}

static ProcReact_Future next_derivation_future(void *data)
{
    /* Declarations */
    DerivationIteratorData *derivation_iterator_data = (DerivationIteratorData*)data;
    
    /* Retrieve derivation item, interface pair */
    DerivationItem *item = g_ptr_array_index(derivation_iterator_data->derivation_array, derivation_iterator_data->model_iterator_data.index);
    Interface *interface = find_interface(derivation_iterator_data->interface_array, item->target);
    
    /* Invoke the next derivation item operation process */
    ProcReact_Future future = derivation_iterator_data->map_derivation_item_function.future(derivation_iterator_data->data, item, interface);
    
    /* Increase the iterator and update the pid table */
    next_iteration_future(&derivation_iterator_data->model_iterator_data, &future, item);
    
    /* Return the pid of the invoked process */
    return future;
}

static void complete_derivation_future(void *data, ProcReact_Future *future, ProcReact_Status status)
{
    DerivationIteratorData *derivation_iterator_data = (DerivationIteratorData*)data;
    
    /* Retrieve the completed item */
    DerivationItem *item = complete_iteration_future(&derivation_iterator_data->model_iterator_data, future, status);
    
    /* Invoke callback that handles the completion of derivation item mapping */
    derivation_iterator_data->complete_derivation_item_mapping_function.future(derivation_iterator_data->data, item, future, status);
}

ProcReact_FutureIterator create_derivation_future_iterator(const GPtrArray *derivation_array, const GPtrArray *interface_array, map_derivation_item_future_function map_derivation_item, complete_derivation_item_mapping_future_function complete_derivation_item_mapping, void *data)
{
    DerivationIteratorData *derivation_iterator_data = create_common_iterator(derivation_array, interface_array, data);
    derivation_iterator_data->map_derivation_item_function.future = map_derivation_item;
    derivation_iterator_data->complete_derivation_item_mapping_function.future = complete_derivation_item_mapping;
    
    return procreact_initialize_future_iterator(has_next_derivation_item, next_derivation_future, complete_derivation_future, derivation_iterator_data);
}

static void destroy_derivation_iterator_data(DerivationIteratorData *derivation_iterator_data)
{
    destroy_model_iterator_data(&derivation_iterator_data->model_iterator_data);
    g_free(derivation_iterator_data);
}

void destroy_derivation_pid_iterator(ProcReact_PidIterator *iterator)
{
    DerivationIteratorData *derivation_iterator_data = (DerivationIteratorData*)iterator->data;
    destroy_derivation_iterator_data(derivation_iterator_data);
}

void destroy_derivation_future_iterator(ProcReact_FutureIterator *iterator)
{
    DerivationIteratorData *derivation_iterator_data = (DerivationIteratorData*)iterator->data;
    destroy_derivation_iterator_data(derivation_iterator_data);
    procreact_destroy_future_iterator(iterator);
}

int derivation_iterator_has_succeeded(const DerivationIteratorData *derivation_iterator_data)
{
    return derivation_iterator_data->model_iterator_data.success;
}
