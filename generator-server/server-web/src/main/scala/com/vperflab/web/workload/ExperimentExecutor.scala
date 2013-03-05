package com.vperflab.web.workload

import com.vperflab.workload.WLApp
import net.liftweb.common.{Logger, Box}
import com.vperflab.web.lib.SpringAdapter._
import com.vperflab.web.lib.DependencyFactory
import java.util.Date
import com.vperflab.model.{Execution, Experiment}

/**
 * @author Dmitry Ivanov (divanov@ambiqtech.ru)
 *         Ambiq Technology Ltd
 */

object ExperimentExecutor {

  private val logger = Logger(this.getClass)
  private val workloadSystem = getBean(classOf[WLApp])

  lazy val currentTime: Box[Date] = DependencyFactory.inject[Date] // inject the date

  def execute(exp: Experiment) {
    val execution = Execution.create.experiment(exp).execStartTime(currentTime.getOrElse(new Date()))
    execution.save()

    val iterations = exp.getIterations

    try {
      logger.debug("Found "+ iterations.length+" iterations... processing")

      for (iter <- iterations){
        logger.debug("Iteration start: " + currentTime.getOrElse(new Date()))

        workloadSystem.runWorkload(execution, iter, iter.clients.is.toInt, iter.clients.is.toInt)

        Thread.sleep(iter.duration.is)

        logger.debug("Iteration end: " + currentTime.getOrElse(new Date()))

      }

    } catch {
      case e: Exception => logger.error(e.getMessage, e)
    }

    val executionEndTime =  currentTime.getOrElse(new Date())

    execution.duration(executionEndTime.getTime - execution.execStartTime.getTime)
    execution.save()
  }

}
