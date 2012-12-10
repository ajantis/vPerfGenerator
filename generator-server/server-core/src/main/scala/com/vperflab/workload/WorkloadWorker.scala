package com.vperflab.workload

import akka.actor.Actor
import akka.util.Duration._
import java.util.concurrent.TimeUnit
import akka.util.{FiniteDuration, Duration}

import org.slf4j.LoggerFactory
import org.apache.commons.httpclient.methods.GetMethod
import org.apache.commons.httpclient.{HttpException, HttpStatus, HttpClient}

import java.io.IOException
/**
 * @author Dmitry Ivanov
 */

class vPerfClient {
  def run() = {
    
  }
}

class WorkloadWorker extends Actor {

  val logger = LoggerFactory.getLogger(getClass)

  def receive = {
    case Work(iterations) =>
      sender ! ResultWaitTime(doRequests(iterations)) // perform the work
  }

  protected def doRequests(numberOfIterations: Int): Duration = {
    val waitTimes = 1.to(numberOfIterations).map {  i =>
      new Request(new vPerfClient()).process() 
    }

    aggregateWaitTimes(waitTimes)  
  }

  protected def aggregateWaitTimes(waitTimes: Seq[Duration]): FiniteDuration = {
    // just a (sum) / count
    Duration.create(
      (waitTimes.foldLeft(0L)((s, waitTime) => s + waitTime.toMillis)) / waitTimes.length,
      TimeUnit.MILLISECONDS
    )
  }

}

class Request(client: vPerfClient) {

  def process(): Duration = {

    val startTime = System.currentTimeMillis

    client.run()

    Duration(System.currentTimeMillis - startTime, TimeUnit.MILLISECONDS)
  }
}