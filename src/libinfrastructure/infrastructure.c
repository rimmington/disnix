/*
 * Disnix - A Nix-based distributed service deployment tool
 * Copyright (C) 2008-2016  Sander van der Burg
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

#include "infrastructure.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <xmlutil.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#define BUFFER_SIZE 4096

static gint compare_target_property(const TargetProperty **l, const TargetProperty **r)
{
    const TargetProperty *left = *l;
    const TargetProperty *right = *r;
    
    return g_strcmp0(left->name, right->name);
}

static gchar *find_target_property(const Target *target, const gchar *name)
{
    TargetProperty key;
    const TargetProperty *key_ptr = &key;
    TargetProperty **ret;
    
    key.name = (gchar*)name;
    
    ret = bsearch(&key_ptr, target->properties->pdata, target->properties->len, sizeof(gpointer), (int (*)(const void *, const void *)) compare_target_property);
    
    if(ret == NULL)
        return NULL;
    else
        return (*ret)->value;
}

static gint compare_target(const Target **l, const Target **r)
{
    const Target *left = *l;
    const Target *right = *r;
    
    gchar *left_target_property = find_target_key(left, NULL);
    gchar *right_target_property = find_target_key(right, NULL);
    
    return g_strcmp0(left_target_property, right_target_property);
}

static gint compare_container(const Container **l, const Container **r)
{
    const Container *left = *l;
    const Container *right = *r;
    
    return g_strcmp0(left->name, right->name);
}

static xmlDocPtr create_infrastructure_doc(gchar *infrastructureXML)
{
    /* Declarations */
    xmlDocPtr doc, transform_doc;
    xmlNodePtr root_node;
    xsltStylesheetPtr style;
    
    /* Parse XML file from XML string */
    doc = xmlParseMemory(infrastructureXML, strlen(infrastructureXML));
    
    if(doc == NULL)
    {
	g_printerr("Error with parsing infrastructure XML file!\n");
	return NULL;
    }
    
    /* Check if the document has a root */
    root_node = xmlDocGetRootElement(doc);
    
    if(root_node == NULL)
    {
        g_printerr("The infrastructure XML file is empty!\n");
	xmlFreeDoc(doc);
	return NULL;
    }

    /* Transform the document into a more concrete format */
    style = xsltParseStylesheetFile((const xmlChar *) DATADIR "/infrastructure.xsl");
    
    transform_doc = xsltApplyStylesheet(style, doc, NULL);
        
    /* Cleanup */
    xsltFreeStylesheet(style);
    xmlFreeDoc(doc);
    xsltCleanupGlobals();
    
    /* Return transformed XML document */
    return transform_doc;
}

static gchar *create_infrastructure_xml(gchar *infrastructure_expr)
{
    int pipefd[2];
        
    /* 
     * Execute nix-instantiate command to retrieve XML representation of the 
     * infrastructure model
     */     
    
    if(pipe(pipefd) == 0)
    {
	int status = fork();
    
	if(status == -1)
	{
	    g_printerr("Error with forking nix-instantiate process!\n");
	    close(pipefd[0]);
	    close(pipefd[1]);
	    return NULL;
	}
	else if(status == 0)
	{
	    char *const args[] = {"nix-instantiate", "--eval-only", "--strict", "--xml", infrastructure_expr, NULL};
	    close(pipefd[0]); /* Close read-end of pipe */
	    dup2(pipefd[1], 1); /* Attach write-end to stdout */
	    execvp("nix-instantiate", args);
	    _exit(1);
	    return NULL;
	}
	else
	{
	    gchar *infrastructureXML = g_strdup("");
	    ssize_t line_size;
	    char line[BUFFER_SIZE];
	    
	    close(pipefd[1]); /* Close write-end of pipe */
		
	    while((line_size = read(pipefd[0], line, BUFFER_SIZE - 1)) > 0)
	    {
	        gchar *old_infrastructureXML = infrastructureXML;

	        line[line_size] = '\0';
		infrastructureXML = g_strconcat(old_infrastructureXML, line, NULL);
		g_free(old_infrastructureXML);
	    }
	    
	    close(pipefd[0]);
	    
	    wait(&status);

	    if(WEXITSTATUS(status) == 0)
		return infrastructureXML;
	    else
	    {
		g_printerr("Error with executing nix-instantiate!\n");
		g_free(infrastructureXML);
		return NULL;
	    }
	}    
    }
    else
    {
	g_printerr("Error with creating pipe!\n");
	return NULL;
    }
}

GPtrArray *create_target_array(char *infrastructure_expr)
{
    /* Declarations */
    gchar *infrastructureXML;
    xmlDocPtr doc;
    xmlXPathObjectPtr result;
    GPtrArray *targets_array = NULL;
    
    /* Open the XML output of nix-instantiate */
    infrastructureXML = create_infrastructure_xml(infrastructure_expr);
    
    if(infrastructureXML == NULL)
    {
        g_printerr("Error opening infrastructure XML file!\n");
        return NULL;
    }
    
    /* Parse the infrastructure XML file */
    doc = create_infrastructure_doc(infrastructureXML);
    g_free(infrastructureXML);
    
    if(doc == NULL)
    {
        g_printerr("Error with parsing infrastructure XML file!\n");
        return NULL;
    }

    /* Query all the target properties from the XML document */
    result = executeXPathQuery(doc, "/infrastructure/target");

    /* Iterate over all targets and add them to the array */
    
    if(result)
    {
        unsigned int i;
        
        xmlNodeSetPtr nodeset = result->nodesetval;
        
        /* Create a targets array */
	targets_array = g_ptr_array_new();
	
	/* Iterate over all the target elements */
	for(i = 0; i < nodeset->nodeNr; i++)
	{
	    xmlNodePtr targets_children = nodeset->nodeTab[i]->children;
	    xmlAttrPtr target_properties = nodeset->nodeTab[i]->properties;
	    Target *target = (Target*)g_malloc(sizeof(Target));
	
	    gchar *name = NULL;
	    gchar *system = NULL;
	    gchar *clientInterface = NULL;
	    gchar *targetProperty = NULL;
	    int numOfCores = 0;
	    int availableCores = 0;
	    GPtrArray *properties = NULL;
	    GPtrArray *containers = NULL;
	    
	    while(target_properties != NULL)
	    {
	        if(xmlStrcmp(target_properties->name, (xmlChar*) "name") == 0)
	        {
	            if(target_properties->children != NULL)
	                name = g_strdup((gchar*)target_properties->children->content);
	        }
	        
	        target_properties = target_properties->next;
	    }
	    
	    while(targets_children != NULL)
	    {
	        if(xmlStrcmp(targets_children->name, (xmlChar*) "system") == 0)
	        {
	            if(targets_children->children != NULL)
	                system = g_strdup((gchar*)targets_children->children->content);
	        }
	        else if(xmlStrcmp(targets_children->name, (xmlChar*) "clientInterface") == 0)
	        {
	            if(targets_children->children != NULL)
	                clientInterface = g_strdup((gchar*)targets_children->children->content);
	        }
	        else if(xmlStrcmp(targets_children->name, (xmlChar*) "targetProperty") == 0)
	        {
	            if(targets_children->children != NULL)
	                targetProperty = g_strdup((gchar*)targets_children->children->content);
	        }
	        else if(xmlStrcmp(targets_children->name, (xmlChar*) "numOfCores") == 0)
	        {
	            if(targets_children->children != NULL)
	            {
	                numOfCores = atoi((char*)targets_children->children->content);
	                availableCores = numOfCores;
	            }
	        }
	        else if(xmlStrcmp(targets_children->name, (xmlChar*) "properties") == 0)
	        {
	            xmlNodePtr properties_children = targets_children->children;
	            properties = g_ptr_array_new();
	            
	            /* Iterate over all properties */
	            while(properties_children != NULL)
	            {
	                TargetProperty *targetProperty = (TargetProperty*)g_malloc(sizeof(TargetProperty));
	                targetProperty->name = g_strdup((gchar*)properties_children->name);
	                
	                if(properties_children->children == NULL)
	                    targetProperty->value = NULL;
	                else
	                    targetProperty->value = g_strdup((gchar*)properties_children->children->content);
	                
	                g_ptr_array_add(properties, targetProperty);
	                
	                properties_children = properties_children->next;
	            }
	            
	            /* Sort the target properties */
	            g_ptr_array_sort(properties, (GCompareFunc)compare_target_property);
	        }
	        else if(xmlStrcmp(targets_children->name, (xmlChar*) "containers") == 0)
	        {
	            xmlNodePtr container_children = targets_children->children;
	            containers = g_ptr_array_new();
	            
	            /* Iterate over all containers */
	            while(container_children != NULL)
	            {
	                Container *container = (Container*)g_malloc(sizeof(Container));
	                container->name = g_strdup((gchar*)container_children->name);
	                
	                if(container_children->children == NULL)
	                    container->properties = NULL;
	                else
	                {
	                    xmlNodePtr properties_children = container_children->children;
	                    GPtrArray *properties = g_ptr_array_new();
	                    
	                    /* Iterate over all properties */
	                    while(properties_children != NULL)
	                    {
	                        TargetProperty *targetProperty = (TargetProperty*)g_malloc(sizeof(TargetProperty));
	                        targetProperty->name = g_strdup((gchar*)properties_children->name);
	                        
	                        if(properties_children->children == NULL)
	                            targetProperty->value = NULL;
	                        else
	                            targetProperty->value = g_strdup((gchar*)properties_children->children->content);
	                
	                        g_ptr_array_add(properties, targetProperty);
	                
	                        properties_children = properties_children->next;
	                    }
	                    
	                    /* Sort the target properties */
	                    g_ptr_array_sort(properties, (GCompareFunc)compare_target_property);
	                    
	                    container->properties = properties;
	                }
	                
	                g_ptr_array_add(containers, container);
	                
	                container_children = container_children->next;
	            }
	            
	            /* Sort the containers */
	            g_ptr_array_sort(containers, (GCompareFunc)compare_container);
	        }
	        
	        targets_children = targets_children->next;
	    }
	    
	    target->name = name;
	    target->system = system;
	    target->clientInterface = clientInterface;
	    target->targetProperty = targetProperty;
	    target->numOfCores = numOfCores;
	    target->availableCores = availableCores;
	    target->properties = properties;
	    target->containers = containers;
	    
	    /* Add target item to the targets array */
	    g_ptr_array_add(targets_array, target);
	}
	
	/* Sort the targets array */
	g_ptr_array_sort(targets_array, (GCompareFunc)compare_target);

	xmlXPathFreeObject(result);
    }
    else
        g_printerr("No targets found!\n");

    /* Cleanup */
    xmlFreeDoc(doc);
    xmlCleanupParser();

    /* Return the target array */
    return targets_array;
}

static void delete_properties(GPtrArray *properties)
{
    if(properties != NULL)
    {
        unsigned int i;
    
        for(i = 0; i < properties->len; i++)
        {
            TargetProperty *targetProperty = g_ptr_array_index(properties, i);
            
            g_free(targetProperty->name);
            g_free(targetProperty->value);
            g_free(targetProperty);
        }
        
        g_ptr_array_free(properties, TRUE);
    }
}

static void delete_containers(GPtrArray *containers)
{
    if(containers != NULL)
    {
        unsigned int i;

        for(i = 0; i < containers->len; i++)
        {
            Container *container = g_ptr_array_index(containers, i);
            g_free(container->name);
            delete_properties(container->properties);
        }
        
        g_ptr_array_free(containers, TRUE);
    }
}

static void delete_target(Target *target)
{
    delete_properties(target->properties);
    delete_containers(target->containers);
    
    g_free(target->system);
    g_free(target->clientInterface);
    g_free(target->targetProperty);
    g_free(target);
}

void delete_target_array(GPtrArray *target_array)
{
    if(target_array != NULL)
    {
        unsigned int i;
        
        for(i = 0; i < target_array->len; i++)
        {
            Target *target = g_ptr_array_index(target_array, i);
            delete_target(target);
        }
    
        g_ptr_array_free(target_array, TRUE);
    }
}

gchar *find_target_key(const Target *target, const gchar *global_target_property)
{
    if(target->targetProperty == NULL)
        return find_target_property(target, global_target_property);
    else
        return find_target_property(target, target->targetProperty);
}
