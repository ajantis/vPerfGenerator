package com.vperflab.web.snippet

import net.liftweb.common.Loggable
import com.vperflab.agent.AgentService
import net.liftweb.util._
import Helpers._
import com.vperflab.model.Agent
import net.liftweb.http.{S, SHtml}
import com.vperflab.web.lib.SpringAdapter._

/**
 * Copyright iFunSoftware 2013
 * @author Dmitry Ivanov
 */
class AgentsSnippet extends Loggable{

  private val agentService = getBean(classOf[AgentService])

  def list = {
    ".agent *" #> agentService.listAgents.map {
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