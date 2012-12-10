package com.vperflab.tsserver;

import java.lang.annotation.*;

@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.FIELD})
public @interface TSObjContainer
{
    Class elementClass();
}