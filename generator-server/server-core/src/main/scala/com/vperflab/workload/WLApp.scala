package com.vperflab.workload

import akka.actor.{Props, ActorSystem}
import org.springframework.stereotype.Component
import com.vperflab.model.{Execution, Iteration, Experiment}
import java.util.Date


/**
 * @author Dmitry Ivanov
 */

@Component
class WLApp {

  def runWorkload(execution: Execution, iteration: Iteration, nrOfWorkers: Int, nrOfMessages: Int) {
    // Create an Akka system
    val system = ActorSystem("WorkloadSystem")

    // create the result listener, which will print the result and shutdown the system
    val listener = system.actorOf(Props(new WorkloadListener(iteration, execution)), name = "WorkloadListener")

    // create the master
    val master = system.actorOf(Props(new WorkloadDirector(
      nrOfWorkers, nrOfMessages, listener)),
      name = "workloadDirector")

    // start the workload
    master ! RunWorkload

  }
}