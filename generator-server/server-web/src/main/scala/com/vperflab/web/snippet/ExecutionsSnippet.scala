package com.vperflab.web.snippet

import net.liftweb.util._
import Helpers._
import net.liftweb.http.S
import net.liftweb.mapper.By
import com.vperflab.model.{IterationExecution, Execution}
import xml.NodeSeq
import net.liftweb.common.{Logger, Full}

/**
 * @author Dmitry Ivanov
 */

class ExecutionsSnippet {

  val logger = Logger(classOf[ExecutionsSnippet])

  def view = {
    S.param("execId") match {
      case Full(execId) => {
        Execution.find(By(Execution.id, execId.toInt)) match {
          case Full(execution: Execution) => {
            ".experiment_name *" #> (execution.experiment.obj.map(_.name.is).openOr("N/A")) &
            ".exec_start *" #> execution.execStartTime.is.toString &
            ".wait_time_graph_render *" #>
                new FlotReport().graph(NodeSeq.Empty, List(getExecWaitTimeVals(execution)), "wait_time_graph_area") &
            ".clients_count_graph_render *" #>
                new FlotReport().stackBar(NodeSeq.Empty, List(getExecClientCountVals(execution)), "clients_count_graph_area") &
            ".iteration_executions *" #> {
            "li *" #> execution.getIterationExecutions.map (
                (iterExec: IterationExecution) => {
                  ".duration" #> (iterExec.execEndTime.is.getTime - iterExec.execStartTime.is.getTime).toString &
                  ".wait_time" #> (iterExec.result.is) &
                  ".clients_count" #> (iterExec.iteration.obj.map(_.clients.is).openOr(0L))
              })
            }
          }
          case _ => {
            S.warning("Execution is not found!")
            PassThru
          }
        }
      }
      case _ => {
        S.error("Execution id is not defined!")
        PassThru
      }
    }
  }

  private def getExecWaitTimeVals(execution: Execution): List[(Double, Double)] = {

    val iterExecs = execution.getIterationExecutions
    val startTime = execution.execStartTime.is.getTime

    val waitTimes = iterExecs.map(
      (iterExec: IterationExecution) =>
        ((iterExec.execStartTime.is.getTime - startTime).toDouble, iterExec.result.is.toDouble)
    )

    waitTimes
  }

  private def getExecClientCountVals(execution: Execution): List[(Double, Double)] = {

    val iterExecs = execution.getIterationExecutions
    val startTime = execution.execStartTime.is.getTime
    val timeXAxis = iterExecs.map( (iterExec: IterationExecution) => (iterExec.execStartTime.is.getTime - startTime).toDouble )

    val iterationsClients = execution.experiment.obj match {
      case Full(exp) => {
        exp.getIterations.map(i => (i.clients.is).toDouble)
      }
      case _ => {
        logger.error("No experiment for execution!")
        Nil
      }
    }
    val clientsPerTime = timeXAxis.zip(iterationsClients)
    clientsPerTime
  }

}
