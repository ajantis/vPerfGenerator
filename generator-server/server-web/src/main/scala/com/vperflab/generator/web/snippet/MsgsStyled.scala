package com.vperflab.generator.web.snippet

import _root_.net.liftweb.http._
import _root_.scala.xml._
import S._

/**
 * @author Dmitry Ivanov (divanov@ambiqtech.ru)
 *         Ambiq Technology Ltd
 */
class MsgsStyled {

  def msgs(cls: String, ms: List[NodeSeq]) = ms match {

    case Nil => Nil
    case x =>
      <div class={ cls }>
        <a class="close" href="#">&times;</a>
        {
          ms.flatMap(m =>
          <p>{ m }</p>)
        }
      </div>
  }

  def render(xml: NodeSeq) : NodeSeq =
    <div id={ LiftRules.noticesContainerId }>
      { msgs("alert-message error", noIdMessages(errors)) }
      { msgs("alert-message warning", noIdMessages(warnings)) }
      { msgs("alert-message info", noIdMessages(notices)) }
    </div>
}
