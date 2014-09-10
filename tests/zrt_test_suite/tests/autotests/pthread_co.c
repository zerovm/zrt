/*
 * simplest pthread test 
 *
 * Copyright (c) 2014, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int condition;
int count;
pthread_t thread;

int consume( void ){
    while ( count < 10 ){
	printf( "Consume wait\n" );
	pthread_mutex_lock( &mutex );
	while( condition == 0 )
	    pthread_cond_wait( &cond, &mutex );
	printf( "Consumed %d\n", count );
	condition = 0;
	pthread_cond_signal( &cond );      
	pthread_mutex_unlock( &mutex );
    }
    return( 0 );
}

void* produce( void * arg ){
    while ( count < 10 ){
	printf( "Produce wait\n" );
	pthread_mutex_lock( &mutex );
	while( condition == 1 )
	    pthread_cond_wait( &cond, &mutex );
	count++;
	printf( "Produced %d\n", count );
	condition = 1;
	pthread_cond_signal( &cond );      
	pthread_mutex_unlock( &mutex );
    }
    return( 0 );
}

int main( void ){
   pthread_create( &thread, NULL, &produce, NULL );
   return consume();
}
