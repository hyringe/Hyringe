package hycallparser.builtinProcedures;

import hycallparser.parameter.Parameter;

import java.util.List;

public interface Procedure {

    String getName();
    int getParameterCount();

    Parameter perform(List<Parameter> args);
}
