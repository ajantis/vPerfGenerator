package com.vperflab.web.snippet

import xml.Text
import net.liftweb.sitemap.Loc.{Link, LinkText}
import net.liftweb.common.{Loggable, Full, Box}
import com.vperflab.model.{WorkloadParamInfo, ModuleInfo, Agent}
import loc.ItemRewriteLoc
import org.bson.types.ObjectId
import net.liftweb.util.BindHelpers._
import net.liftweb.http.SHtml
import com.vperflab.web.lib.SpringAdapter._
import com.vperflab.agent.AgentService
import net.liftweb.http.js.JsCmds

/**
 * @author Dmitry Ivanov (id.ajantis@gmail.com)
 * iFunSoftware 2013
 */
object AgentPage extends ItemRewriteLoc[Agent, AgentPageData] {

  override val name = "Agent"
  override def title = Text(currentValue.map(_.agent.hostName.is).openOr("Agent"))
  override def text = new LinkText[AgentPageData](text = v => Text(v.agent.hostName.is))

  override val pathPrefix = "agents" :: Nil
  override lazy val pathList = pathPrefix ++ List("index")
  override def link = new Link[AgentPageData](pathList){
    override def pathList(value: AgentPageData): List[String] = pathPrefix ::: value.agent.id.is.toString :: Nil
  }
  override def getItem(id: String) = Agent.find("_id", new ObjectId(id))
  override def wrapItem(agentBox: Box[Agent]) = agentBox.map(AgentPageData(_))
  override def canonicalUrl(data: AgentPageData) = {
    Full((pathPrefix:::List(data.agent.id.is.toString)).mkString("/","/",""))
  }
}

class AgentPage(data: AgentPageData) extends Loggable{

  private val agentService = getBean(classOf[AgentService])

  def view = {
    ".hostname *" #> data.agent.hostName.is &
    ".status *" #> (data.agent.isActive.is match {
      case true => "Online"
      case _ => "Offline"
    }) &
    ".update_info_btn *" #> SHtml.ajaxButton("Update info", () => {agentService.prefetchAgentInfo(data.agent); JsCmds._Noop}) &
    ".module *" #> (data.agent.modules.get.map { module: ModuleInfo =>
      ".name *" #> module.name.is &
      ".path *" #> module.path.is &
      ".params *" #> (module.params.get.map { param: WorkloadParamInfo =>
        ".name *" #> param.name.is &
        ".description *" #> param.description.is &
        ".additional *" #> (param.additionalData.get.map {
          case (key, data) => ".key *" #> key & ".data *" #> data
        })
      })
    })
  }
}
