package com.vperflab.web.snippet

import net.liftweb.util._
import Helpers._
import net.liftweb.http.{S, SHtml}
import com.vperflab.web.workload.ExperimentExecutor
import xml.NodeSeq
import net.liftweb.common.Full
import net.liftweb.mapper.By
import net.liftweb.http.js.JsCmds
import com.vperflab.model.{Execution, Experiment}

/**
 * @author Dmitry Ivanov (divanov@ambiqtech.ru)
 *         Ambiq Technology Ltd
 */

class Experiments {

  def list = {
    "li *" #> experiments.map( (e: Experiment) => {
      "a *" #> e.name.is &
      "a [href]" #> ("/experiments/" + e.id.is.toString)
    })
  }

  def view = {

    S.param("expId") match {
      case Full(expId) => {
        Experiment.find(By(Experiment.id, expId.toInt)) match {
          case Full(experiment) => {
            ".experiment_name *" #> experiment.name.is &
            ".iterations_count *" #> experiment.getIterations.length &
            ".executions *" #> {
              "li *" #> experiment.getExecutions.map ( (exec: Execution) =>
                ".startTime *" #> exec.execStartTime.is.toString
              )
            } &
            ".executions *" #> {
              "li *" #> experiment.getExecutions.map( (exec: Execution) => {
                "a *" #> exec.execStartTime.is.toString &
                "a [href]" #> ("/experiments/executions/" + exec.id.is.toString)
              })
            } &
            ".run_btn [onclick]" #> SHtml.ajaxInvoke( () => { runExperiment(experiment); JsCmds.Alert("Running!") }) &
            ".del_btn [onclick]" #> SHtml.ajaxInvoke( () => {
              deleteExperiment(experiment) match {
                case true => (JsCmds.Alert("Experiment is deleted.") & JsCmds.RedirectTo("/experiments/"))
                case _ => (JsCmds.Alert("Experiment is not deleted!") & JsCmds.RedirectTo(""))
              }
            })
          }
          case _ => {
            S.warning("Experiment is not found!")
            PassThru
          }
        }
      }
      case _ => {
        S.error("Experiment id is not defined!")
        PassThru
      }
    }
  }

  def add = {
    var name = ""
    var iterationDuration = 0L
    var clientsCountArr = Array[Long]()
    var immediateRun = false

    def processClientsCountArr(s: String) = {
      s.split(',').map(_.toLong)
    }

    ".name" #> SHtml.onSubmit( name = _ ) &
    ".iteration_duration" #> SHtml.onSubmit(s => iterationDuration = s.toLong) &
    ".run_immediately" #> SHtml.checkbox(immediateRun, immediateRun = _ ) &
    ".clients_count_arr" #> SHtml.onSubmit(s => clientsCountArr = processClientsCountArr(s) ) &
    ":submit" #> SHtml.onSubmitUnit( () => create(name, iterationDuration, clientsCountArr, immediateRun) )
  }

  private def create(experimentName: String, iterDuration: Long, clientsCountArr: Array[Long], immediateRun: Boolean = false) {
    if (experimentName != ""){
      val exp = Experiment.createAndSave(experimentName, iterDuration, clientsCountArr)

      if(immediateRun)
        runExperiment(exp)

      S.notice("Experiment is submitted")
    }
    else {
      S.error("Experiment name cannot be empty!")
    }
  }

  private def experiments: List[Experiment] = Experiment.findAll()

  private def runExperiment(exp: Experiment){
    ExperimentExecutor.execute(exp)
  }

  private def deleteExperiment(exp: Experiment) = {
    Experiment.delete_!(exp)
  }
}
