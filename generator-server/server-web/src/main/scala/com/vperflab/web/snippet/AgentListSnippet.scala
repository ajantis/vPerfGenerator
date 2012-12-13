package com.vperflab.web.snippet

import net.liftweb.util._
import Helpers._

import net.liftweb.http.S

import net.liftweb.common.{Logger, Full}

import com.vperflab.agent._

/**
 * @author Dmitry Ivanov
 */

class AgentListSnippet {

  val logger = Logger(classOf[ExecutionsSnippet])
  
  def list = {
    "td *" #> AgentService.listAgents.map( (agent: AgentStatus) => {
      "a *" #> agent.hostName &
      "a [href]" #> ("/agent/" + agent.hostName)
    })
  }
  
}