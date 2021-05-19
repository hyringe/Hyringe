package hycallparser.builtinProcedures;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterInteger;
import hycallparser.parameter.ParameterList;

import java.math.BigInteger;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.LongStream;

public class ProcedureRange implements Procedure {
    @Override
    public String getName() {
        return "range";
    }

    @Override
    public int getParameterCount() {
        return 2;
    }

    @Override
    public Parameter perform(List<Parameter> args) {
        Parameter lower = args.get(0);
        Parameter upper = args.get(1);

        if (!(lower.isInteger() && upper.isInteger()))
            throw new IllegalArgumentException("Builtin procedure range expects two integers as parameters!");

        return new ProcedureRangeStep().perform(List.of(lower, new ParameterInteger(BigInteger.ONE), upper));
    }
}
