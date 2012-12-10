package com.vperflab.tsserver;

import java.lang.annotation.*;

@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.TYPE})
public @interface TSObjAbstract
{
	String 	 fieldName();
    String[] classMap();
}