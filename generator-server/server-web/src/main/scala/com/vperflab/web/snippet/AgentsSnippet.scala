package com.vperflab.web.snippet

import net.liftweb.common.Loggable
import com.vperflab.agent.AgentService
import net.liftweb.util._
import Helpers._
import com.vperflab.model.Agent
import net.liftweb.http.{S, SHtml}

/**
 * Copyright iFunSoftware 2013
 * @author Dmitry Ivanov
 */
class AgentsSnippet extends Loggable{

  def list = {
    ".agent *" #> AgentService.listAgents.map {
      case agent: Agent =>
        "a *" #> agent.hostName &
        "a [href]" #> agent.url &
        ".status *" #> agent.isActive
    }
  }

  def newAgent = {
    val newAgent = Agent.createRecord

    def createAgent(){
      newAgent.validate match {
        case Nil => {
          newAgent.save
          S.redirectTo(Agent.baseAgentsUrl)
          S.notice("Agent " + newAgent.hostName + " is created")
        }
        case xs => S.error(xs)
      }
    }

    ".hostname" #> SHtml.onSubmit(newAgent.hostName(_)) &
    ":submit" #> SHtml.onSubmitUnit(createAgent _)
  }

}