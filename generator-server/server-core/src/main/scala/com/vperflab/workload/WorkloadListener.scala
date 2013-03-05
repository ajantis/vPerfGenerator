package com.vperflab.workload

import akka.actor.Actor
import java.util.Date
import com.vperflab.model.{Iteration, Execution, IterationExecution}

/**
 * @author Dmitry Ivanov
 */

class WorkloadListener(iteration: Iteration, execution: Execution) extends Actor {

  def receive = {
    case WaitTimeApproximation(waitTime, duration) => {

      val currentTime = new Date()

      val execStat = IterationExecution.create.execEndTime(currentTime).
        execStartTime(new Date(currentTime.getTime - duration.length)).execution(execution).iteration(iteration).result(waitTime.length)

      execStat.save()

      println("\n\tWait time approximation: \t\t%s\n\tWorking time: \t%s"
        .format(waitTime, duration))

      context.system.shutdown()
    }
  }

}
