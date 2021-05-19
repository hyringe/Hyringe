package hycallparser.visitors_and_listeners;

import hycallparser.antlr.HycallParser;
import hycallparser.parameter.Parameter;

import java.util.HashMap;
import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

public class ProcedureUtils {

    private ProcedureUtils() {}

    public static List<String> getParameters(HycallParser.ProcedureContext pc) {
        return pc.parameterlistDeclare().IDENTIFIER().stream().map(Objects::toString).collect(Collectors.toList());
    }

    public static List<Parameter> getParameterValues(HycallParser.Expression_PROCCALLContext pcc, ExpressionEvaluator ee) {
        return pcc.parameterlistCall().expression().stream().map(ee::visit).collect(Collectors.toList());
    }
}
