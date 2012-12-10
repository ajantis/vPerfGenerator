package com.vperflab.workload

import akka.routing.RoundRobinRouter
import akka.actor.{ActorRef, Props, Actor}
import akka.util.Duration
import java.util.concurrent.TimeUnit
import collection.mutable.ArrayBuffer
import com.vperflab.model.{Iteration, Execution, Experiment}


/**
 * @author Dmitry Ivanov
 */
class WorkloadDirector(nrOfWorkers: Int, nrOfMessages: Int, listener: ActorRef) extends Actor {

  var waitTimes: ArrayBuffer[Duration] = ArrayBuffer()
  var nrOfResults: Int = _
  val start: Long = System.currentTimeMillis

  val workerRouter = context.actorOf(
    Props[WorkloadWorker].withRouter(RoundRobinRouter(nrOfWorkers)), name = "WorkerDirector")


  def receive = {
    case RunWorkload =>
      for (i <- 0 until nrOfMessages) workerRouter ! Work(1)

    case ResultWaitTime(value: Duration) =>
      waitTimes += value
      nrOfResults += 1

      if (nrOfResults == nrOfMessages) {
        // Send the result to the listener
        listener ! WaitTimeApproximation(approximateWaitTime(waitTimes), wlDuration = Duration(System.currentTimeMillis - start, TimeUnit.MILLISECONDS))
        // Stops this actor and all its supervised children
        context.stop(self)
      }
  }

  protected def approximateWaitTime(waitTime: Seq[Duration]): Duration = {
    // just a (sum) / count
    Duration.create(
      (waitTimes.foldLeft(0L)((s, waitTime) => s + waitTime.toMillis)) / waitTimes.length,
      TimeUnit.MILLISECONDS
    )
  }

}
